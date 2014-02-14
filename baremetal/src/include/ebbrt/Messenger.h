//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_MESSENGER_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_MESSENGER_H_

#include <ebbrt/CacheAligned.h>
#include <ebbrt/Future.h>
#include <ebbrt/Net.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/SpinLock.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {
class Messenger : public StaticSharedEbb<Messenger>, public CacheAligned {
  struct Header {
    uint64_t length;
  };

 public:
  class NetworkId {
   public:
    explicit NetworkId(uint32_t ip_h) { ip.addr = htonl(ip_h); }

   private:
    struct ip_addr ip;

    friend class Messenger;
  };

  Messenger();

  Future<void> Send(NetworkId to, std::shared_ptr<const Buffer> data) {
    {
      std::lock_guard<SpinLock> lock(lock_);
      auto it = connection_map_.find(to.ip.addr);
      if (it == connection_map_.end()) {
        // we don't have a pending connection, start one
        NetworkManager::TcpPcb pcb;
        auto f = pcb.Connect(&to.ip, port_);
        auto f2 =
            f.Then(MoveBind([](NetworkManager::TcpPcb pcb, Future<void> f) {
                              kprintf("Connected\n");
                              f.Get();
                              return pcb;
                            },
                            std::move(pcb)));
        connection_map_.emplace(to.ip.addr, std::move(f2));
      }
    }
    return connection_map_[to.ip.addr].Then(
        MoveBind([](std::shared_ptr<const Buffer> data,
                    Future<NetworkManager::TcpPcb> f) {
                   kprintf("Sending %d\n", data->total_size());
                   auto buf = Buffer(sizeof(Header));
                   auto h = static_cast<Header*>(buf.data());
                   h->length = data->total_size();
                   buf.append(std::move(std::const_pointer_cast<Buffer>(data)));
                   f.Get().Send(std::make_shared<const Buffer>(std::move(buf)));
                 },
                 std::move(data)));
  }

 private:
  void StartListening(uint16_t port);

  static uint16_t port_;
  NetworkManager::TcpPcb tcp_;
  ebbrt::SpinLock lock_;
  // FIXME: This should be a shared future!
  std::unordered_map<uint32_t, Future<NetworkManager::TcpPcb>> connection_map_;

  friend void ebbrt::runtime::Init();
};

constexpr auto messenger = EbbRef<Messenger>(kMessengerId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_MESSENGER_H_
