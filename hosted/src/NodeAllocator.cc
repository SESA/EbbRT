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

  auto f = popen((std::string("docker network create ") +
                  DefaultNetworkArguments + std::to_string(std::time(nullptr)))
                     .c_str(),
                 "r");
  if (f == nullptr) {
    throw std::runtime_error("popen failed");
  }
  char line[100];
  while (fgets(line, 100, f) != nullptr) {
    network_id_ += line;
  }
  network_id_.erase(network_id_.length() - 1);  // trim newline
  pclose(f);

  auto ipct =
      std::string("docker network inspect ") +
      std::string(" --format='{{range .IPAM.Config}}{{.Gateway}}{{end}}' ") +
      network_id_;
  auto g = popen(ipct.c_str(), "r");

  std::string str;
  while (fgets(line, 100, g) != nullptr) {
    str += line;
  }
  uint8_t ip0, ip1, ip2, ip3;
  sscanf(str.c_str(), "%hhu.%hhu.%hhu.%hhu/%hhu", &ip0, &ip1, &ip2, &ip3,
         &cidr_);
  pclose(g);
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
    if (system(cmd.c_str()) < 0) {
      std::cout << "Error: command failed - " << cmd << std::endl;
    }
  }

  std::cout << "Node Allocator bound to " << ipaddr << ":" << port_
            << std::endl;
  std::cout << "network: " << network_id_.substr(0, 12) << std::endl;
  DoAccept(std::move(acceptor), std::move(socket));
}

ebbrt::NodeAllocator::~NodeAllocator() {
  for (auto n : nodes_) {
    std::string c = "docker logs " + n;
    int rc = system(c.c_str());
    if (rc < 0) {
      std::cout << "ERROR: system rc =" << rc << " for command: " << c
                << std::endl;
    }
  }
  for (auto n : nodes_) {
    std::string c = "docker rm -f " + n;
    std::cout << "Removing container " << n.substr(0, 12) << std::endl;
    auto f = popen(c.c_str(), "r");
    if (f == nullptr) {
      throw std::runtime_error("'docker container rm' failed");
    }
    pclose(f);
  }
  {
    std::cout << "Removing network " << network_id_.substr(0, 12) << std::endl;
    std::string c = "docker network rm " + network_id_;
    auto f = popen(c.c_str(), "r");
    if (f == nullptr) {
      throw std::runtime_error("'docker network rm' failed");
    }
    pclose(f);
  }
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

ebbrt::NodeAllocator::NodeDescriptor
ebbrt::NodeAllocator::AllocateNode(std::string binary_path, int cpus,
                                   int numaNodes, int ram,
                                   std::string arguments) {
  auto fdt = Fdt();
  fdt.BeginNode("/");
  fdt.BeginNode("runtime");
  fdt.CreateProperty("address", net_addr_);
  auto port = port_;
  fdt.CreateProperty("port", port);
  auto allocation_id =
      node_allocator->allocation_index_.fetch_add(1, std::memory_order_relaxed);
  fdt.CreateProperty("allocation_id", allocation_id);
  fdt.EndNode();
  fdt.EndNode();
  fdt.Finish();

  // Write Fdt
  auto dir = boost::filesystem::temp_directory_path();
  auto fname = dir / boost::filesystem::unique_path();
  if (boost::filesystem::exists(fname)) {
    throw std::runtime_error("Failed to create unique temporary file name");
  }
  // cid file
  auto cid = dir / boost::filesystem::unique_path();
  if (boost::filesystem::exists(fname)) {
    throw std::runtime_error(
        "Failed to create container id temporary file name");
  }

  // TODO(dschatz): make this asynchronous?
  boost::filesystem::create_directories(dir);
  std::ofstream outfile(fname.native());
  const char* fdt_addr = fdt.GetAddr();
  size_t fdt_size = fdt.GetSize();
  outfile.write(fdt_addr, fdt_size);
  outfile.flush();
  if (!outfile.good()) {
    throw std::runtime_error("Failed to write fdt");
  }

  std::string cmd =
      std::string(" docker run -td -P --cap-add NET_ADMIN") +
      std::string(" --device  /dev/kvm:/dev/kvm") +
      std::string(" --device /dev/net/tun:/dev/net/tun") +
      std::string(" --device /dev/vhost-net:/dev/vhost-net") +
      std::string(" --net=") + network_id_ + std::string(" -e VM_MEM=") +
      std::to_string(ram) + std::string("G") + std::string(" -e VM_CPU=") +
      std::to_string(cpus) + std::string(" -e VM_WAIT=true") +
      std::string(" --cidfile=") + cid.native()
#ifndef NDEBUG
      + std::string(" --expose 1234") + std::string(" ebbrt/kvm-qemu:debug") +
      std::string(" --gdb tcp:0.0.0.0:1234 ")
#else
      + std::string(" ebbrt/kvm-qemu:latest")
#endif
      + arguments + std::string(" -kernel /root/img.elf") +
      std::string(" -initrd /root/initrd");

  std::cout << "Starting container... " << std::endl;
  auto f = popen(cmd.c_str(), "r");
  if (f == nullptr) {
    throw std::runtime_error("Failed to create container");
  }
  char line[kLineSize];
  std::string node_id;
  while (std::fgets(line, kLineSize, f)) {
    node_id += line;
  }
  node_id.erase(node_id.length() - 1);  // trim newline
  pclose(f);
  // inspect for IpAddr
  auto getip = "docker inspect -f '{{range "
               ".NetworkSettings.Networks}}{{.IPAddress}}{{end}}' " +
               node_id;
  auto g = popen(getip.c_str(), "r");
  if (g == nullptr) {
    throw std::runtime_error("Failed to get node ip ");
  }
  std::string ip;
  while (fgets(line, kLineSize, g) != nullptr) {
    ip += line;
  }
  ip.erase(ip.length() - 1);  // trim newline
  pclose(g);

#ifndef NDEBUG
  auto lpcmd = std::string("/bin/sh -c \"docker port ") + node_id +
               std::string(" | grep 1234 | cut -d ':' -f 2\"");
  auto e = popen(lpcmd.c_str(), "r");
  if (e == nullptr) {
    throw std::runtime_error("Failed to get local gdb port");
  }
  std::string lport;
  while (fgets(line, kLineSize, e) != nullptr) {
    lport += line;
  }
  lport.erase(lport.length() - 1);  // trim newline
  pclose(e);
#endif

  /* transfer images into container */
  std::string sk = std::string("scp -q -o UserKnownHostsFile=/dev/null -o "
                               "StrictHostKeyChecking=no ") +
                   binary_path + std::string(" root@") + ip +
                   std::string(":/root/img.elf");
  std::string si = std::string("scp -q -o UserKnownHostsFile=/dev/null -o "
                               "StrictHostKeyChecking=no ") +
                   fname.native() + std::string(" root@") + ip +
                   std::string(":/root/initrd");

  if (system(sk.c_str()) < 0) {
    std::cout << "Error: command failed - " << sk << std::endl;
    throw std::runtime_error("Image transfer failed.");
  }
  if (system(si.c_str()) < 0) {
    std::cout << "Error on command: " << si << std::endl;
    throw std::runtime_error("Image transfer failed.");
  }

  /* starting vm */
  std::string kick = std::string("docker exec -dt ") + node_id +
                     std::string(" touch /tmp/signal");
  if (system(kick.c_str()) < 0) {
    std::cout << "Error: command failed - " << kick << std::endl;
    throw std::runtime_error("VM creation failed.");
  }

  nodes_.emplace_back(node_id);
  auto rfut = promise_map_[allocation_id].GetFuture();
  std::cout << "fdt: " << fname.native() << std::endl;
#ifndef NDEBUG
  std::cout << "gdb local: localhost:" << lport << std::endl;
  std::cout << "gdb remote: " << ip << ":1234" << std::endl;
#endif
  std::cout << "container: " << node_id.substr(0, 12) << std::endl;
  return NodeDescriptor(node_id, std::move(rfut));
}

void ebbrt::NodeAllocator::FreeNode(std::string node_id) {
  std::string command = "docker rm -f " + node_id;
  std::cout << "Removing container " << node_id << std::endl;
  auto f = popen(command.c_str(), "r");
  if (f == nullptr) {
    throw std::runtime_error("Failed to free node");
  }
  nodes_.erase(std::remove(nodes_.begin(), nodes_.end(), node_id),
               nodes_.end());
}

uint32_t ebbrt::NodeAllocator::GetNetAddr() { return net_addr_; }
