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
  static const constexpr int kDefaultCpus=2; 
  static const constexpr int kDefaultNumaNodes=2;
  static const constexpr int kDefaultRam=2;  
  static int DefaultCpus;
  static int DefaultNumaNodes;
  static int DefaultRam;
 public:

  static void InitDefaults();

  NodeAllocator();
  ~NodeAllocator();

  class NodeDescriptor {
   public:
    NodeDescriptor(uint16_t node_id,
                   ebbrt::Future<ebbrt::Messenger::NetworkId> net_id)
        : node_id_(node_id), net_id_(std::move(net_id)) {}
    uint16_t NodeId() { return node_id_; }
    ebbrt::Future<ebbrt::Messenger::NetworkId>& NetworkId() { return net_id_; }

   private:
    uint16_t node_id_;
    ebbrt::Future<ebbrt::Messenger::NetworkId> net_id_;
  };
  NodeDescriptor AllocateNode(std::string binary_path) {
    return AllocateNode(binary_path, DefaultCpus, DefaultNumaNodes,
		 DefaultRam);
  }
  NodeDescriptor AllocateNode(std::string binary_path, 
			      int cpus,
			      int numNodes,
			      int ram);
  void FreeNode(uint16_t node_id);

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
  std::atomic<uint16_t> allocation_index_;
  std::unordered_map<uint16_t, Promise<Messenger::NetworkId>> promise_map_;
  int network_id_;
  uint32_t net_addr_;
  uint16_t port_;
  char dir_[160];

  friend class Session;
  friend class Messenger;
};
const constexpr auto node_allocator = EbbRef<NodeAllocator>(kNodeAllocatorId);
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_NODEALLOCATOR_H_
