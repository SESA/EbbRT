//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef NDEBUG
#include <sys/socket.h>
#endif
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../CapnpMessage.h"
#include "Debug.h"
#include "Messenger.h"
#include "NodeAllocator.h"
#include <RuntimeInfo.capnp.h>
#include <boost/filesystem.hpp>
#include <capnp/message.h>

namespace bai = boost::asio::ip;
const constexpr size_t kLineSize = 80;

// Node Configuration
uint8_t ebbrt::NodeAllocator::DefaultCpus;
uint8_t ebbrt::NodeAllocator::DefaultRam;
std::string ebbrt::NodeAllocator::DefaultArguments;
// Network Configuration
std::string ebbrt::NodeAllocator::CustomNetworkCreate;
std::string ebbrt::NodeAllocator::CustomNetworkRemove;
std::string ebbrt::NodeAllocator::CustomNetworkIp;
std::string ebbrt::NodeAllocator::CustomNetworkNodeArguments;

void
ebbrt::NodeAllocator::ClassInit() {
  // Node create configuration
  char* str = getenv("EBBRT_NODE_ALLOCATOR_DEFAULT_CPUS");
  DefaultCpus = (str) ? atoi(str) : kDefaultCpus;
  str = getenv("EBBRT_NODE_ALLOCATOR_DEFAULT_RAM");
  DefaultRam = (str) ? atoi(str) : kDefaultRam;
  str = getenv("EBBRT_NODE_ALLOCATOR_DEFAULT_ARGUMENTS");
  DefaultArguments = (str) ? std::string(str) : std::string();
  // Network create configuration
  str = getenv("EBBRT_NODE_ALLOCATOR_CUSTOM_NETWORK_CREATE_CMD");
  CustomNetworkCreate = (str) ? std::string(str) : std::string();
  str = getenv("EBBRT_NODE_ALLOCATOR_CUSTOM_NETWORK_REMOVE_CMD");
  CustomNetworkRemove = (str) ? std::string(str) : std::string();
  str = getenv("EBBRT_NODE_ALLOCATOR_CUSTOM_NETWORK_IP_CMD");
  CustomNetworkIp = (str) ? std::string(str) : std::string();
  str = getenv("EBBRT_NODE_ALLOCATOR_CUSTOM_NETWORK_NODE_CONFIG");
  CustomNetworkNodeArguments = (str) ? std::string(str) : std::string();
}

std::string ebbrt::NodeAllocator::RunCmd(std::string cmd) {
  std::string out;
  char line[kLineSize];
  if (cmd.empty()) {
    return std::string();
  }
  auto f = popen(cmd.c_str(), "r");
  if (f == nullptr) {
    throw std::runtime_error("Failed to execute command: " + cmd);
  }
  while (std::fgets(line, kLineSize, f)) {
    out += line;
  }
  if (pclose(f) != 0) {
    throw std::runtime_error("Error reported by command: " + cmd + out);
  }
  if (!out.empty())
    out.erase(out.length() - 1);  // trim newline
  return out;
}

ebbrt::NodeAllocator::DockerContainer::~DockerContainer() {
  if (!cid_.empty()) {
    std::cerr << "removing container: " << cid_.substr(0, 12) << std::endl;
    RunCmd(base_+" stop " + cid_);
    RunCmd(base_+" rm -f " + cid_);
  }
}

std::string ebbrt::NodeAllocator::DockerContainer::GetIp() {
  auto cip = RunCmd(base_+" inspect -f '{{range "
                    ".NetworkSettings.Networks}}{{.IPAddress}}{{end}}' " +
                    cid_ );
  assert(cip != "");
  return cip;
}

std::string ebbrt::NodeAllocator::DockerContainer::Start() {
  if (!cid_.empty() || img_.size() == 0) {
    throw std::runtime_error("Error: attempt to start unspecified container ");
  }
  std::stringstream cmd;
  cmd << base_ << " run -d " << arg_ << " " << img_ << " " << cmd_;
  cid_ = RunCmd(cmd.str());
  std::cerr << "Docker Container:" << std::endl;
  if( host_ != ""){
    std::cerr << "| host:" << host_ << std::endl;
  }
  std::cerr << "|  cid: " << cid_.substr(0, 12) << std::endl;
  std::cerr << "|  log: " << base_ << " logs " << cid_.substr(0, 12) << std::endl;
  return cid_;
}

std::string ebbrt::NodeAllocator::DockerContainer::StdOut() {
  ebbrt::kbugon(cid_.empty());
  return RunCmd(base_+" logs " + cid_);
}

ebbrt::NodeAllocator::Session::Session(bai::tcp::socket socket,
                                       uint32_t net_addr)
    : socket_(std::move(socket)), net_addr_(net_addr) {}

namespace {
class IOBufToCBS {
 public:
  explicit IOBufToCBS(std::unique_ptr<const ebbrt::IOBuf>&& buf)
      : buf_(std::move(buf)) {
    buf_vec_.reserve(buf_->CountChainElements());
    for (auto& b : *buf_) {
      buf_vec_.emplace_back(b.Data(), b.Length());
    }
  }

  typedef boost::asio::const_buffer value_type;
  typedef std::vector<boost::asio::const_buffer>::const_iterator const_iterator;

  const_iterator begin() const { return buf_vec_.cbegin(); }
  const_iterator end() const { return buf_vec_.cend(); }

 private:
  std::shared_ptr<const ebbrt::IOBuf> buf_;
  std::vector<boost::asio::const_buffer> buf_vec_;
};

}  // namespace

void ebbrt::NodeAllocator::Session::Start() {
  IOBufMessageBuilder message;
  auto builder = message.initRoot<RuntimeInfo>();

  // set message
  auto index =
      node_allocator->node_index_.fetch_add(1, std::memory_order_relaxed);
  builder.setEbbIdSpace(index);
  builder.setMessengerPort(messenger->GetPort());
  builder.setGlobalIdMapAddress(net_addr_);

  auto self(shared_from_this());
  async_write(
      socket_, IOBufToCBS(AppendHeader(message)),
      EventManager::WrapHandler([this,
                                 self](const boost::system::error_code& error,
                                       std::size_t /*bytes_transferred*/) {
        if (error) {
          throw std::runtime_error("Node allocator failed to configure node");
        }
        auto val = new uint16_t;
        async_read(socket_, boost::asio::buffer(reinterpret_cast<char*>(val),
                                                sizeof(uint16_t)),
                   EventManager::WrapHandler(
                       [this, self, val](const boost::system::error_code& error,
                                         std::size_t /*bytes_transferred*/) {
                         if (error) {
                           throw std::runtime_error(
                               "Node allocator failed to configure node");
                         }
                         auto v = ntohs(*val);
                         auto& pmap = node_allocator->promise_map_;
                         assert(pmap.find(v) != pmap.end());
                         pmap[v].SetValue(Messenger::NetworkId(
                             socket_.remote_endpoint().address().to_v4()));
                         pmap.erase(v);
                       }));
      }));
}

ebbrt::NodeAllocator::NodeAllocator() : node_index_(2), allocation_index_(0) {
  auto acceptor = std::make_shared<bai::tcp::acceptor>(
      active_context->io_service_, bai::tcp::endpoint(bai::tcp::v4(), 0));
  auto socket = std::make_shared<bai::tcp::socket>(active_context->io_service_);
  std::string network_ip;
  if (CustomNetworkCreate.empty() && CustomNetworkIp.empty()) {
    auto net_name =
        "ebbrt-" + std::to_string(geteuid()) + "." + std::to_string(getpid());
    network_id_ = RunCmd(std::string("docker network create " + net_name));
    network_ip = RunCmd("docker network inspect --format='{{range "
                        ".IPAM.Config}}{{.Gateway}}{{end}}' " +
                        network_id_);
  } else {
    std::cerr << "Creating custom network" << std::endl;
    network_id_ = RunCmd(CustomNetworkCreate);
    network_ip = RunCmd(CustomNetworkIp + network_id_);
  }

  uint8_t ip0, ip1, ip2, ip3;
  sscanf(network_ip.c_str(), "%hhu.%hhu.%hhu.%hhu\\%hhu", &ip0, &ip1, &ip2,
         &ip3, &cidr_);
  net_addr_ = ip0 << 24 | ip1 << 16 | ip2 << 8 | ip3;
  port_ = acceptor->local_endpoint().port();
  auto ipaddr = std::to_string(static_cast<int>(ip0)) + "." +
                std::to_string(static_cast<int>(ip1)) + "." +
                std::to_string(static_cast<int>(ip2)) + "." +
                std::to_string(static_cast<int>(ip3));
  AppendArgs("host_addr=" + std::to_string(net_addr_));
  AppendArgs("host_port=" + std::to_string(port_));
  std::cerr << "Network Details:" << std::endl;
  std::cerr << "| id: " << network_id_.substr(0, 12) << std::endl;
  std::cerr << "| ip: " << ipaddr << ":" << port_ << std::endl;
#ifndef NDEBUG
  std::cerr << "# debug w/ wireshark:" << std::endl;
  std::cerr << "# wireshark -i br-" << network_id_.substr(0, 12) << " -k"
            << std::endl;
#endif
  DoAccept(std::move(acceptor), std::move(socket));
}

std::string ebbrt::NodeAllocator::AllocateContainer(std::string repo,
                                                    std::string container_args,
                                                    std::string run_cmd) {
  // start arbitrary container on our network
  container_args += " --net=" + network_id_;
  auto c = DockerContainer(repo, container_args, run_cmd);
  auto id = c.Start();
  nodes_.insert(std::make_pair(std::string(id), std::move(c)));
  return RunCmd("docker inspect -f '{{range "
                ".NetworkSettings.Networks}}{{.IPAddress}}{{end}}' " +
                id);
}

void ebbrt::NodeAllocator::DoAccept(
    std::shared_ptr<bai::tcp::acceptor> acceptor,
    std::shared_ptr<bai::tcp::socket> socket) {
  acceptor->async_accept(
      *socket, EventManager::WrapHandler([this, acceptor, socket](
                   boost::system::error_code ec) {
        if (!ec) {
          std::make_shared<Session>(std::move(*socket), net_addr_)->Start();
          DoAccept(std::move(acceptor), std::move(socket));
        }
      }));
}

void ebbrt::NodeAllocator::AppendArgs(std::string arg) {
  if (!cmdline_.empty()) {
    cmdline_ += ";";
  }
  cmdline_ += arg;
}

ebbrt::NodeAllocator::NodeDescriptor
ebbrt::NodeAllocator::AllocateNode(std::string binary_path, 
    const ebbrt::NodeAllocator::NodeArgs& args){

  assert(args.cpus != 0);
  assert(args.ram != 0);

  RunCmd("file " + binary_path);
  auto allocation_id =
      node_allocator->allocation_index_.fetch_add(1, std::memory_order_relaxed);
  auto container_name = "ebbrt-" + std::to_string(geteuid()) + "." +
                        std::to_string(getpid()) + "." +
                        std::to_string(allocation_id);
  std::stringstream docker_args;
  std::stringstream qemu_args;

#ifndef NDEBUG
  docker_args << " --expose 1234 -e DEBUG=true";
#endif

  if (CustomNetworkNodeArguments.empty()) {
    docker_args << " --net=" << network_id_ << " ";
  } else {
    docker_args << " " << CustomNetworkNodeArguments << " ";
  }
  docker_args << " -td -P --cap-add NET_ADMIN"
              << " --device /dev/kvm:/dev/kvm"
              << " --device /dev/net/tun:/dev/net/tun"
              << " --device /dev/vhost-net:/dev/vhost-net"
              << " -e VM_WAIT=true"
              << " -e VM_MEM=" << std::to_string(args.ram) << "G"
              << " -e VM_CPU=" << std::to_string(args.cpus)
              << " --name='" << container_name
              << "' ";

  std::string repo = " ebbrt/kvm-qemu:latest";

#ifndef NDEBUG
  qemu_args << " --gdb tcp:0.0.0.0:1234 ";
#endif
  qemu_args << args.arguments << " -kernel /root/img.elf"
            << " -append \"" << cmdline_ << ";allocid=" << allocation_id
            << "\"";

  auto c = DockerContainer(repo, docker_args.str(), qemu_args.str(), args.constraint_node);
  auto id = c.Start();
  auto cip = c.GetIp(); /* get IP of container */

  nodes_.insert(std::make_pair(std::string(id), std::move(c)));
  auto rfut = promise_map_[allocation_id].GetFuture();

  /* transfer image into container */
  RunCmd("ping -c 3 -w 30 " + cip);
  RunCmd("nc -z -w 30 " + cip + " 22");
  RunCmd("scp -q -o UserKnownHostsFile=/dev/null -o " 
         "StrictHostKeyChecking=no " +
         binary_path + " root@" + cip + ":/root/img.elf");
  RunCmd("ssh -q -o UserKnownHostsFile=/dev/null -o "
         "StrictHostKeyChecking=no root@"+cip+" 'touch /tmp/signal'");

  std::cerr << "Node Allocation Details: " << std::endl;
  std::cerr << "| img: " << binary_path << std::endl;
  std::cerr << "|  id: " << container_name << std::endl;
  std::cerr << "|  ip: " << cip << std::endl;
#ifndef NDEBUG
  std::cerr << "# debug w/ gdb: " << std::endl;
  std::cerr << "# gdb " << binary_path.substr(0, binary_path.size() - 2)
            << " -q -ex 'set tcp connect-timeout 60' -ex 'target remote " << cip
            << ":1234'" << std::endl;
#endif
  return NodeDescriptor(id, std::move(rfut));
}
ebbrt::NodeAllocator::~NodeAllocator() {
  nodes_.clear();
  if (CustomNetworkRemove.empty()) {
    std::cerr << "removing Network: " << network_id_.substr(0, 12) << std::endl;
    RunCmd("docker network rm " + network_id_);
  } else {
    std::cerr << "Removing custom network" << std::endl;
    RunCmd(CustomNetworkRemove + network_id_);
  }
}

void ebbrt::NodeAllocator::FreeNode(std::string node_id) {
  nodes_.erase(node_id);
}

uint32_t ebbrt::NodeAllocator::GetNetAddr() { return net_addr_; }
