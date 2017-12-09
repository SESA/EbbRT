//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_NODEALLOCATOR_H_
#define HOSTED_SRC_INCLUDE_EBBRT_NODEALLOCATOR_H_

#include <string>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>

#include "../StaticSharedEbb.h"
#include "EbbRef.h"
#include "Messenger.h"
#include "StaticIds.h"

namespace ebbrt {
class NodeAllocator : public StaticSharedEbb<NodeAllocator> {
  struct DockerContainer {
    DockerContainer() = delete; 
    DockerContainer(std::string img,
                    std::string arg = std::string(),
                    std::string cmd = std::string(), 
                    std::string host = std::string())
        : arg_{arg}, cmd_{cmd}, host_{host}, img_{img} {
            base_ = "docker";
            if(host_ != ""){
              base_ += " -H "+host_;
            }
          }
    ~DockerContainer();
    DockerContainer(DockerContainer&& other) noexcept /* move constructor */
        : arg_{std::move(other.arg_)},
          base_{std::move(other.base_)},
          cid_{std::move(other.cid_)},
          cmd_{std::move(other.cmd_)},
          host_{std::move(other.host_)},
          img_{std::move(other.img_)} {}
    DockerContainer&
    operator=(DockerContainer&& other) noexcept /* move assignment */ {
      arg_ = std::move(other.arg_);
      base_ = std::move(other.base_);
      cid_ = std::move(other.cid_);
      cmd_ = std::move(other.cmd_);
      host_ = std::move(other.host_);
      img_ = std::move(other.img_);
      return *this;
    }
    DockerContainer(const DockerContainer& other) =
        delete; /* copy constructor */
    DockerContainer&
    operator=(const DockerContainer& other) = delete; /* copy assignment */
    std::string Start();
    std::string StdOut();
    std::string GetIp();
    void Stop() {}

   private:
    std::string arg_ = std::string();
    std::string base_ = std::string();
    std::string cid_ = std::string();
    std::string cmd_ = std::string();
    std::string host_ = std::string();
    std::string img_ = std::string();
  };

  static const constexpr uint8_t kDefaultCpus = 2;
  static const constexpr uint8_t kDefaultRam = 2;
  static uint8_t DefaultCpus;
  static uint8_t DefaultRam;
  static std::string DefaultArguments;
  static std::string CustomNetworkCreate;
  static std::string CustomNetworkRemove;
  static std::string CustomNetworkIp;
  static std::string CustomNetworkNodeArguments;

 public:
  static void ClassInit();  // Class wide static initialization logic

  static std::string RunCmd(std::string cmd);
  NodeAllocator();
  ~NodeAllocator();

  struct NodeArgs {
    uint8_t cpus = DefaultCpus;
    uint8_t ram = DefaultRam;
    std::string arguments = DefaultArguments;
    std::string constraint_node = "";
    NodeArgs() {} 
  };

  class NodeDescriptor {
   public:
    NodeDescriptor(std::string node_id,
                   ebbrt::Future<ebbrt::Messenger::NetworkId> net_id)
        : node_id_(node_id), net_id_(std::move(net_id)) {}
    std::string NodeId() { return node_id_; }
    ebbrt::Future<ebbrt::Messenger::NetworkId>& NetworkId() { return net_id_; }

   private:
    std::string node_id_;
    ebbrt::Future<ebbrt::Messenger::NetworkId> net_id_;
  };
  NodeDescriptor AllocateNode(std::string binary_path, const NodeArgs& args = NodeArgs());

  std::string AllocateContainer(std::string repo,
                                std::string container_args = std::string(),
                                std::string run_cmd = std::string());
  void AppendArgs(std::string arg);
  void FreeNode(std::string node_id);

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
  std::string GetId() { return network_id_; }

  std::atomic<uint16_t> node_index_;
  std::atomic<uint16_t> allocation_index_;
  std::unordered_map<uint16_t, Promise<Messenger::NetworkId>> promise_map_;
  std::string network_id_;
  std::string cmdline_;
  uint32_t net_addr_;
  uint16_t port_;
  uint8_t cidr_;
  std::unordered_map<std::string, DockerContainer> nodes_;

  friend class Session;
  friend class Messenger;
};
const constexpr auto node_allocator = EbbRef<NodeAllocator>(kNodeAllocatorId);
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_NODEALLOCATOR_H_
