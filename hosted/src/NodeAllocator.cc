//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <fstream>

#include <libfdt.h>

#include <boost/filesystem.hpp>
#include <capnp/message.h>

#include <ebbrt/Fdt.h>
#include <ebbrt/GlobalMap.h>
#include <ebbrt/NodeAllocator.h>

#include <RuntimeInfo.capnp.h>

namespace bai = boost::asio::ip;

namespace {
struct array_ptr_wrapper {
  explicit array_ptr_wrapper(const kj::ArrayPtr<const capnp::word>* aptr)
      : aptr_(aptr) {}

  const kj::ArrayPtr<const capnp::word>* aptr_;

  bool operator==(const array_ptr_wrapper& rhs) { return aptr_ == rhs.aptr_; }
  bool operator!=(const array_ptr_wrapper& rhs) { return !(*this == rhs); }
  array_ptr_wrapper& operator++() {
    aptr_++;
    return *this;
  }
  const boost::asio::const_buffer& operator*() {
    return boost::asio::buffer(static_cast<const void*>(aptr_->begin()),
                               aptr_->size() * sizeof(capnp::word));
  }
};

class aptr_to_cbs {
 public:
  typedef boost::asio::const_buffer value_type;
  typedef array_ptr_wrapper const_iterator;
  explicit aptr_to_cbs(kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> aptr)
      : aptr_(aptr) {}
  const_iterator begin() const { return array_ptr_wrapper(aptr_.begin()); }
  const_iterator end() const { return array_ptr_wrapper(aptr_.end()); }

 private:
  kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> aptr_;
};
}  // namespace

ebbrt::NodeAllocator::Session::Session(bai::tcp::socket socket,
                                       uint32_t net_addr)
    : socket_(std::move(socket)), net_addr_(net_addr) {
  std::cout << "New connection" << std::endl;
  auto message = new capnp::MallocMessageBuilder();
  auto builder = message->initRoot<RuntimeInfo>();

  // set message
  auto index =
      node_allocator->node_index_.fetch_add(1, std::memory_order_relaxed);
  builder.setEbbIdSpace(index);
  builder.setGlobalMapPort(global_map->GetPort());
  builder.setGlobalMapAddress(net_addr_);

  auto size = builder.totalSize().wordCount * sizeof(capnp::word);
  auto segments = message->getSegmentsForOutput();
  socket_.async_send(aptr_to_cbs(segments),
                     [message, size](const boost::system::error_code& error,
                                     std::size_t /*bytes_transferred*/) {
    delete message;
    if (error) {
      throw std::runtime_error("Node allocator failed to configure node");
    }
  });
}

ebbrt::NodeAllocator::NodeAllocator()
    : node_index_(2), acceptor_(active_context->io_service_,
                                bai::tcp::endpoint(bai::tcp::v4(), 0)),
      socket_(active_context->io_service_) {
  // TODO(dschatz): fix this hard coded name
  auto f = popen("/opt/khpy/kh network", "r");
  if (f == nullptr) {
    throw std::runtime_error("popen failed");
  }

  std::string str;
  char line[100];
  while (fgets(line, 100, f) != nullptr) {
    str += line;
  }
  uint8_t ip0, ip1, ip2, ip3;
  sscanf(str.c_str(), "%d\n%hhu.%hhu.%hhu.%hhu\n", &network_id_, &ip0, &ip1,
         &ip2, &ip3);
  pclose(f);
  net_addr_ = ip0 << 24 | ip1 << 16 | ip2 << 8 | ip3;
  std::cout << "Node Allocator bound to " << static_cast<int>(ip0) << "."
            << static_cast<int>(ip1) << "." << static_cast<int>(ip2) << "."
            << static_cast<int>(ip3) << ":" << acceptor_.local_endpoint().port()
            << std::endl;
  DoAccept();
}

void ebbrt::NodeAllocator::DoAccept() {
  acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
    if (!ec) {
      std::make_shared<Session>(std::move(socket_), net_addr_);
    }
    DoAccept();
  });
}

void ebbrt::NodeAllocator::AllocateNode(std::string binary_path) {
  auto fdt = Fdt();
  fdt.BeginNode("/");
  fdt.BeginNode("runtime");
  fdt.CreateProperty("address", net_addr_);
  auto port = acceptor_.local_endpoint().port();
  fdt.CreateProperty("port", port);
  fdt.EndNode();
  fdt.EndNode();
  fdt.Finish();

  // Write Fdt
  auto dir = boost::filesystem::temp_directory_path() / "ebbrt";
  auto fname = dir / boost::filesystem::unique_path();
  if (boost::filesystem::exists(fname)) {
    throw std::runtime_error("Failed to create unique temporary file name");
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

  // file should flush here
  std::cout << "Fdt written to " << fname.native() << std::endl;

  char network[100];
  snprintf(network, sizeof(network), "%d", network_id_);
  std::string command = "/opt/khpy/kh alloc " + std::string(network) + " " +
                        binary_path + " " + fname.native();
  std::cout << "executing " << command << std::endl;
  system(command.c_str());
}
