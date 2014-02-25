//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_NODEALLOCATOR_H_
#define HOSTED_SRC_INCLUDE_EBBRT_NODEALLOCATOR_H_

#include <string>

#include <boost/asio.hpp>

#include <ebbrt/EbbRef.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {
class NodeAllocator : public StaticSharedEbb<NodeAllocator> {
 public:
  NodeAllocator();
  ~NodeAllocator();

  void AllocateNode(std::string binary_path);

 private:
  class Session : public std::enable_shared_from_this<Session> {
   public:
    explicit Session(boost::asio::ip::tcp::socket socket, uint32_t net_addr);
    void Start();

   private:
    boost::asio::ip::tcp::socket socket_;
    uint32_t net_addr_;
  };

  void DoAccept(std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor,
                std::shared_ptr<boost::asio::ip::tcp::socket> socket);
  uint32_t GetNetAddr();

  std::atomic<uint16_t> node_index_;
  int network_id_;
  uint32_t net_addr_;
  uint16_t port_;

  friend class Session;
  friend class Messenger;
};
const constexpr auto node_allocator = EbbRef<NodeAllocator>(kNodeAllocatorId);
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_NODEALLOCATOR_H_
