//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <cstdint>

#include <fdt.h>

#include <capnp/message.h>

#include <ebbrt/Config.h>
#include <ebbrt/GlobalMap.h>
#include <ebbrt/NodeAllocator.h>

#include <RuntimeInfo.capnp.h>

namespace bai = boost::asio::ip;

namespace {
struct array_ptr_wrapper {
  explicit array_ptr_wrapper(const kj::ArrayPtr<const capnp::word> *aptr)
      : aptr_(aptr) {}

  const kj::ArrayPtr<const capnp::word> *aptr_;

  bool operator==(const array_ptr_wrapper &rhs) { return aptr_ == rhs.aptr_; }
  bool operator!=(const array_ptr_wrapper &rhs) { return !(*this == rhs); }
  array_ptr_wrapper &operator++() {
    aptr_++;
    return *this;
  }
  const boost::asio::const_buffer &operator*() {
    return boost::asio::buffer(static_cast<const void *>(aptr_->begin()),
                               aptr_->size() * sizeof(capnp::word));
  }
};

class aptr_to_cbs {
 public:
  typedef boost::asio::const_buffer value_type;
  typedef array_ptr_wrapper const_iterator;
  explicit aptr_to_cbs(
      kj::ArrayPtr<const kj::ArrayPtr<const capnp::word> > aptr)
      : aptr_(aptr) {}
  const_iterator begin() const { return array_ptr_wrapper(aptr_.begin()); }
  const_iterator end() const { return array_ptr_wrapper(aptr_.end()); }

 private:
  kj::ArrayPtr<const kj::ArrayPtr<const capnp::word> > aptr_;
};
}  // namespace

ebbrt::NodeAllocator::Session::Session(bai::tcp::socket socket)
    : socket_(std::move(socket)) {
  std::cout << "New connection" << std::endl;
  auto message = new capnp::MallocMessageBuilder();
  auto builder = message->initRoot<RuntimeInfo>();

  // set message
  auto index =
      node_allocator->node_index_.fetch_add(1, std::memory_order_relaxed);
  builder.setEbbIdSpace(index);
  auto addr = global_map->GetAddress();
  builder.setGlobalMapPort(ntohs(addr.first));
  builder.setGlobalMapAddress(ntohl(addr.second));

  auto size = builder.totalSize().wordCount * sizeof(capnp::word);
  auto segments = message->getSegmentsForOutput();
  socket_.async_send(aptr_to_cbs(segments),
                     [message, size](const boost::system::error_code &error,
                                     std::size_t /*bytes_transferred*/) {
    delete message;
    if (error) {
      throw std::runtime_error("Node allocator failed to configure node");
    }
  });
}

ebbrt::NodeAllocator::NodeAllocator()
    : node_index_(2),
      acceptor_(
          active_context->io_service_,
          bai::tcp::endpoint(bai::address_v4::from_string(frontend_ip), 0)),
      socket_(active_context->io_service_) {
  std::cout << "Node Allocator bound to "
            << acceptor_.local_endpoint().address() << ":"
            << acceptor_.local_endpoint().port() << std::endl;
  DoAccept();
}

void ebbrt::NodeAllocator::DoAccept() {
  acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
    if (!ec) {
      std::make_shared<Session>(std::move(socket_));
    }
    DoAccept();
  });
}

void ebbrt::NodeAllocator::AllocateNode() {
  
  std::cout << "Allocate node" << std::endl;
}
