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

#include <RuntimeInfo.capnp.h>
#include <boost/filesystem.hpp>
#include <capnp/message.h>
#include <ebbrt/CapnpMessage.h>
#include <ebbrt/Debug.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/NodeAllocator.h>

namespace bai = boost::asio::ip;
const constexpr size_t kLineSize = 80;

int ebbrt::NodeAllocator::DefaultCpus;
int ebbrt::NodeAllocator::DefaultRam;
int ebbrt::NodeAllocator::DefaultNumaNodes;
std::string ebbrt::NodeAllocator::DefaultArguments;
std::string ebbrt::NodeAllocator::DefaultNetworkArguments;

std::string ebbrt::NodeAllocator::RunCmd(std::string cmd) {
#ifndef NDEBUG
  std::cerr << "exec: " << cmd << std::endl;
#endif
  ebbrt::kbugon(cmd.size() == 0);
  std::string out;
  char line[kLineSize];
  auto f = popen(cmd.c_str(), "r");
  if (f == nullptr) {
    throw std::runtime_error("Failed to run command: " + cmd);
  }
  while (std::fgets(line, kLineSize, f)) {
    out += line;
  }
  pclose(f);
  if (!out.empty()) {
    out.erase(out.length() - 1);  // trim newline
  }
  return out;
}

ebbrt::NodeAllocator::DockerContainer::~DockerContainer() {
  if (!cid_.empty()) {
    std::cerr << "Removing container " << cid_.substr(0, 12) << std::endl;
    RunCmd("docker stop " + cid_);
    RunCmd("docker rm -f " + cid_);
  }
}

std::string ebbrt::NodeAllocator::DockerContainer::Start() {
  if (!cid_.empty() || img_.size() == 0) {
    throw std::runtime_error("Error: attempt to start unspecified container ");
    return std::string();
  }
  std::stringstream cmd;
  cmd << "docker run -d " << arg_ << " " << img_ << " " << cmd_;
  cid_ = RunCmd(cmd.str());
  return cid_;
}

std::string ebbrt::NodeAllocator::DockerContainer::StdOut() {
  ebbrt::kbugon(cid_.empty());
  return RunCmd("docker logs " + cid_);
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
  {
    char* str = getenv("EBBRT_NODE_ALLOCATOR_DEFAULT_CPUS");
    DefaultCpus = (str) ? atoi(str) : kDefaultCpus;
    str = getenv("EBBRT_NODE_ALLOCATOR_DEFAULT_RAM");
    DefaultRam = (str) ? atoi(str) : kDefaultRam;
    str = getenv("EBBRT_NODE_ALLOCATOR_DEFAULT_NUMANODES");
    DefaultNumaNodes = (str) ? atoi(str) : kDefaultNumaNodes;
    str = getenv("EBBRT_NODE_ALLOCATOR_DEFAULT_ARGUMENTS");
    DefaultArguments = (str) ? std::string(str) : std::string(" ");
    str = getenv("EBBRT_NODE_ALLOCATOR_DEFAULT_NETWORK_ARGUMENTS");
    DefaultNetworkArguments = (str) ? std::string(str) : std::string(" ");
  }
  auto acceptor = std::make_shared<bai::tcp::acceptor>(
      active_context->io_service_, bai::tcp::endpoint(bai::tcp::v4(), 0));
  auto socket = std::make_shared<bai::tcp::socket>(active_context->io_service_);

  network_id_ = RunCmd(std::string("docker network create " +
                                   std::to_string(std::time(nullptr))));
  std::string str = RunCmd("docker network inspect --format='{{range "
                           ".IPAM.Config}}{{.Gateway}}{{end}}' " +
                           network_id_);
  uint8_t ip0, ip1, ip2, ip3;
  sscanf(str.c_str(), "%hhu.%hhu.%hhu.%hhu\\%hhu", &ip0, &ip1, &ip2, &ip3,
         &cidr_);
  net_addr_ = ip0 << 24 | ip1 << 16 | ip2 << 8 | ip3;
  port_ = acceptor->local_endpoint().port();
  auto ipaddr = std::to_string(static_cast<int>(ip0)) + "." +
                std::to_string(static_cast<int>(ip1)) + "." +
                std::to_string(static_cast<int>(ip2)) + "." +
                std::to_string(static_cast<int>(ip3));

  // optional: support for "weave" docker network plugin
  if (DefaultNetworkArguments.find("weave ") != std::string::npos) {
    RunCmd("weave expose " + ipaddr + "/" + std::to_string(cidr_));
  }

  AppendArgs("host_addr=" + std::to_string(net_addr_));
  AppendArgs("host_port=" + std::to_string(port_));
  std::cout << "Network Details:" << std::endl;
  std::cout << "| network: " << network_id_.substr(0, 12) << std::endl;
  std::cout << "| listening on: " << ipaddr << ":" << port_ << std::endl;
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
ebbrt::NodeAllocator::AllocateNode(std::string binary_path, int cpus,
                                   int numaNodes, int ram,
                                   std::string arguments) {
  auto allocation_id =
      node_allocator->allocation_index_.fetch_add(1, std::memory_order_relaxed);
  auto dir = boost::filesystem::temp_directory_path();

  // cid file
  auto cid = dir / boost::filesystem::unique_path();
  if (boost::filesystem::exists(cid)) {
    throw std::runtime_error(
        "Failed to create container id temporary file name");
  }

  // TODO(dschatz): make this asynchronous?
  boost::filesystem::create_directories(dir);

  std::stringstream docker_args;
  std::stringstream qemu_args;
#ifndef NDEBUG
  docker_args << " --expose 1234";
#endif
  docker_args << " -td -P --cap-add NET_ADMIN"
              << " --device  /dev/kvm:/dev/kvm"
              << " --device /dev/net/tun:/dev/net/tun"
              << " --device /dev/vhost-net:/dev/vhost-net"
              << " --net=" << network_id_ << " -e VM_MEM=" << ram << "G"
              << " -e VM_CPU=" << cpus << " -e VM_WAIT=true"
              << " --cidfile=" << cid.native();

  std::string repo =
#ifndef NDEBUG
      " ebbrt/kvm-qemu:debug";
#else
      " ebbrt/kvm-qemu:latest";
#endif

#ifndef NDEBUG
  qemu_args << " --gdb tcp:0.0.0.0:1234 ";
#endif
  qemu_args << arguments << " -kernel /root/img.elf"
            << " -append \"" << cmdline_ << ";allocid=" << allocation_id
            << "\"";

  auto c = DockerContainer(repo, docker_args.str(), qemu_args.str());
  auto id = c.Start();

  nodes_.insert(std::make_pair(std::string(id), std::move(c)));
  auto rfut = promise_map_[allocation_id].GetFuture();

  /* get IP of container */
  auto cip = RunCmd("docker inspect -f '{{range "
                    ".NetworkSettings.Networks}}{{.IPAddress}}{{end}}' " +
                    id);

#ifndef NDEBUG
  std::string lport = RunCmd("/bin/sh -c \"docker port " + id +
                             " | grep 1234 | cut -d ':' -f 2\"");
#endif

  /* transfer image into container */
  RunCmd("ping -c 1 -w 30 " + cip);
  RunCmd("scp -q -o UserKnownHostsFile=/dev/null -o "
         "StrictHostKeyChecking=no " +
         binary_path + " root@" + cip + ":/root/img.elf");
  /* kick start vm */
  RunCmd("docker exec -dt " + id + " touch /tmp/signal");

  std::cout << "Node Allocation Details: " << std::endl;
#ifndef NDEBUG
  std::cout << "| gdb: " << cip << ":1234" << std::endl;
  std::cout << "| gdb(local): localhost:" << lport << std::endl;
#endif
  std::cout << "| container: " << id.substr(0, 12) << std::endl;
  return NodeDescriptor(id, std::move(rfut));
}
ebbrt::NodeAllocator::~NodeAllocator() {
  nodes_.clear();
  std::cerr << "Removing network " << network_id_.substr(0, 12) << std::endl;
  RunCmd("docker network rm " + network_id_);

  // optional: support for "weave" docker network plugin
  if (DefaultNetworkArguments.find("weave ") != std::string::npos) {
    auto netaddr = ntohl(net_addr_);
    auto ipaddr =
        Messenger::NetworkId::FromBytes(
            reinterpret_cast<const unsigned char*>(&netaddr), sizeof(netaddr))
            .ToString();
    auto cmd = "weave hide " + ipaddr + "/" + std::to_string(cidr_);
    if (system(cmd.c_str()) < 0) {
      std::cout << "Error: command failed - " << cmd << std::endl;
    }
  }
}

void ebbrt::NodeAllocator::FreeNode(std::string node_id) {
  nodes_.erase(node_id);
}

uint32_t ebbrt::NodeAllocator::GetNetAddr() { return net_addr_; }
