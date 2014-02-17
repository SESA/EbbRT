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
    uint64_t type_code;
    EbbId id;
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

  Future<void> Send(NetworkId nid, EbbId id, uint64_t type_code,
                    std::shared_ptr<const Buffer> data);
  void Receive(NetworkManager::TcpPcb& t, Buffer b);

 private:
  void StartListening(uint16_t port);

  static uint16_t port_;
  NetworkManager::TcpPcb tcp_;
  ebbrt::SpinLock lock_;
  std::unordered_map<uint32_t, SharedFuture<NetworkManager::TcpPcb>>
  connection_map_;

  friend void ebbrt::runtime::Init();
};

constexpr auto messenger = EbbRef<Messenger>(kMessengerId);

}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_MESSENGER_H_
