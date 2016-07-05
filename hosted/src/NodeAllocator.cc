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
#include <ebbrt/Fdt.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/NodeAllocator.h>
#include <libfdt.h>

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
    std::string stop = std::string("docker stop ") + cid_;
    std::string rm = std::string("docker rm -f ") + cid_;
    RunCmd(stop);
    RunCmd(rm);
  }
}

std::string ebbrt::NodeAllocator::DockerContainer::Start() {
  if (!cid_.empty() || img_.size() == 0) {
    throw std::runtime_error("Error: attempt to start unspecified container ");
    return std::string();
  }
  std::string cmd = std::string("docker run -d ") + arg_ + std::string(" ") +
                    img_ + std::string(" ") + cmd_;
  cid_ = RunCmd(cmd);
  return cid_;
}

std::string ebbrt::NodeAllocator::DockerContainer::StdOut() {
  ebbrt::kbugon(cid_.empty());
  return RunCmd(std::string("docker logs ") + cid_);
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

ebbrt::NodeAllocator::NodeAllocator()
    : node_index_(2), allocation_index_(0), cmdline_{std::string()} {
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
  std::string str = RunCmd(
      std::string("docker network inspect ") +
      std::string(" --format='{{range .IPAM.Config}}{{.Gateway}}{{end}}' ") +
      network_id_);
  uint8_t ip0, ip1, ip2, ip3;
  sscanf(str.c_str(), "%hhu.%hhu.%hhu.%hhu\\%hhu", &ip0, &ip1, &ip2, &ip3,
         &cidr_);
  net_addr_ = ip0 << 24 | ip1 << 16 | ip2 << 8 | ip3;
  port_ = acceptor->local_endpoint().port();
  auto ipaddr = std::to_string(static_cast<int>(ip0)) + std::string(".") +
                std::to_string(static_cast<int>(ip1)) + std::string(".") +
                std::to_string(static_cast<int>(ip2)) + std::string(".") +
                std::to_string(static_cast<int>(ip3));

  // optional: support for "weave" docker network plugin
  if (DefaultNetworkArguments.find("weave ") != std::string::npos) {
    auto cmd = std::string("weave expose ") + ipaddr + std::string("/") +
               std::to_string(cidr_);
    RunCmd(cmd);
  }
  AppendArgs(std::string("host_addr=") + std::to_string(net_addr_));
  AppendArgs(std::string("host_port=") + std::to_string(port_));

  std::cout << "Network Details:" << std::endl;
  std::cout << "| network: " << network_id_.substr(0, 12) << std::endl;
  std::cout << "| listening on: " << ipaddr << ":" << port_ << std::endl;
  DoAccept(std::move(acceptor), std::move(socket));
}

std::string ebbrt::NodeAllocator::AllocateContainer(std::string repo,
                                                    std::string container_args,
                                                    std::string run_cmd) {
  // start arbitrary container on our network
  container_args += std::string(" --net=") + network_id_;
  auto c = DockerContainer(repo, container_args, run_cmd);
  auto id = c.Start();
  nodes_.insert(std::make_pair(std::string(id), std::move(c)));
  auto getip =
      std::string("docker inspect -f '{{range "
                  ".NetworkSettings.Networks}}{{.IPAddress}}{{end}}' ") +
      id;
  return RunCmd(getip); /* get IP of container */
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
    cmdline_ += std::string(";");
  }
  cmdline_ += arg;
}

ebbrt::NodeAllocator::NodeDescriptor
ebbrt::NodeAllocator::AllocateNode(std::string binary_path, int cpus,
                                   int numaNodes, int ram,
                                   std::string arguments) {
  auto allocation_id =
      node_allocator->allocation_index_.fetch_add(1, std::memory_order_relaxed);
  // Write Fdt
  auto dir = boost::filesystem::temp_directory_path();
#if __EBBRT_ENABLE_FDT__
  auto fdt = Fdt();
  fdt.BeginNode("/");
  fdt.BeginNode("runtime");
  fdt.CreateProperty("address", net_addr_);
  auto port = port_;
  fdt.CreateProperty("port", port);
  fdt.CreateProperty("allocation_id", allocation_id);
  fdt.EndNode();
  fdt.EndNode();
  fdt.Finish();

  auto fname = dir / boost::filesystem::unique_path();
  if (boost::filesystem::exists(fname)) {
    throw std::runtime_error("Failed to create unique temporary file name");
  }
#endif

  // cid file
  auto cid = dir / boost::filesystem::unique_path();
  if (boost::filesystem::exists(cid)) {
    throw std::runtime_error(
        "Failed to create container id temporary file name");
  }

  // TODO(dschatz): make this asynchronous?
  boost::filesystem::create_directories(dir);
#if __EBBRT_ENABLE_FDT__
  std::ofstream outfile(fname.native());
  const char* fdt_addr = fdt.GetAddr();
  size_t fdt_size = fdt.GetSize();
  outfile.write(fdt_addr, fdt_size);
  outfile.flush();
  if (!outfile.good()) {
    throw std::runtime_error("Failed to write fdt");
  }
#endif

  std::string docker_args =
#ifndef NDEBUG
      std::string(" --expose 1234") +
#endif
      std::string(" -td -P --cap-add NET_ADMIN") +
      std::string(" --device  /dev/kvm:/dev/kvm") +
      std::string(" --device /dev/net/tun:/dev/net/tun") +
      std::string(" --device /dev/vhost-net:/dev/vhost-net") +
      std::string(" --net=") + network_id_ + std::string(" -e VM_MEM=") +
      std::to_string(ram) + std::string("G") + std::string(" -e VM_CPU=") +
      std::to_string(cpus) + std::string(" -e VM_WAIT=true") +
      std::string(" --cidfile=") + cid.native();

  std::string repo =
#ifndef NDEBUG
      std::string(" ebbrt/kvm-qemu:debug");
#else
      std::string(" ebbrt/kvm-qemu:latest");
#endif

  std::string qemu_args =
#ifndef NDEBUG
      std::string(" --gdb tcp:0.0.0.0:1234 ") +
#endif
      arguments +
#if __EBBRT_ENABLE_FDT__
      std::string(" -initrd /root/initrd") +
#endif
      std::string(" -kernel /root/img.elf");

  auto cmdline = cmdline_;
  cmdline += std::string(";allocid=") + std::to_string(allocation_id);
  qemu_args += std::string(" -append \"") + cmdline + std::string("\"");

  auto c = DockerContainer(repo, docker_args, qemu_args);
  auto id = c.Start();

  nodes_.insert(std::make_pair(std::string(id), std::move(c)));
  auto rfut = promise_map_[allocation_id].GetFuture();

  auto getip =
      std::string("docker inspect -f '{{range "
                  ".NetworkSettings.Networks}}{{.IPAddress}}{{end}}' ") +
      id;
  auto cip = RunCmd(getip); /* get IP of container */

#ifndef NDEBUG
  auto lpcmd = std::string("/bin/sh -c \"docker port ") + id +
               std::string(" | grep 1234 | cut -d ':' -f 2\"");
  std::string lport = RunCmd(lpcmd);
#endif

  /* transfer images into container */
  std::string sndk = std::string("scp -q -o UserKnownHostsFile=/dev/null -o "
                                 "StrictHostKeyChecking=no ") +
                     binary_path + std::string(" root@") + cip +
                     std::string(":/root/img.elf");
#if __EBBRT_ENABLE_FDT__
  std::string sndi = std::string("scp -q -o UserKnownHostsFile=/dev/null -o "
                                 "StrictHostKeyChecking=no ") +
                     fname.native() + std::string(" root@") + cip +
                     std::string(":/root/initrd");
#endif

  std::string kick =
      std::string("docker exec -dt ") + id + std::string(" touch /tmp/signal");
  RunCmd(sndk); /* send kernel to container */
#if __EBBRT_ENABLE_FDT__
  RunCmd(sndi); /* send initrd to container */
#endif
  RunCmd(kick); /* kick start vm */

  std::cout << "Node Allocation Details: " << std::endl;
#if __EBBRT_ENABLE_FDT__
  std::cout << "| fdt: " << fname.native() << std::endl;
#endif
#ifndef NDEBUG
  std::cout << "| gdb: " << cip << ":1234" << std::endl;
  std::cout << "| gdb(local): localhost:" << lport << std::endl;
#endif
  std::cout << "| container: " << id.substr(0, 12) << std::endl;
  return NodeDescriptor(id, std::move(rfut));
}
ebbrt::NodeAllocator::~NodeAllocator() {
  std::string rmnet = std::string("docker network rm ") + network_id_;
  nodes_.clear();
  std::cerr << "Removing network " << network_id_.substr(0, 12) << std::endl;
  RunCmd(rmnet);

  // optional: support for "weave" docker network plugin
  if (DefaultNetworkArguments.find("weave ") != std::string::npos) {
    auto netaddr = ntohl(net_addr_);
    auto ipaddr =
        Messenger::NetworkId::FromBytes(
            reinterpret_cast<const unsigned char*>(&netaddr), sizeof(netaddr))
            .ToString();
    auto cmd = std::string("weave hide ") + ipaddr + std::string("/") +
               std::to_string(cidr_);
    if (system(cmd.c_str()) < 0) {
      std::cout << "Error: command failed - " << cmd << std::endl;
    }
  }
}

void ebbrt::NodeAllocator::FreeNode(std::string node_id) {
  nodes_.erase(node_id);
}

uint32_t ebbrt::NodeAllocator::GetNetAddr() { return net_addr_; }
