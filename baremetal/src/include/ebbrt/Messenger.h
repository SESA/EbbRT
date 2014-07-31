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
    explicit NetworkId(std::string str) {
      if (str.size() != 4)
        throw std::runtime_error(
            "Cannot build NetworkId from string, size mismatch");

      ip.addr = *reinterpret_cast<const uint32_t*>(str.data());
    }
    bool operator==(const NetworkId& rhs) { return ip.addr == rhs.ip.addr; }

    static NetworkId FromBytes(const unsigned char* p, size_t len) {
      if (unlikely(len != 4))
        throw std::runtime_error("NetworkId::FromBytes length != 4");

      return NetworkId(ntohl(*reinterpret_cast<const uint32_t*>(p)));
    }

    std::string ToBytes() {
      return std::string(reinterpret_cast<const char*>(&ip.addr),
                         sizeof(ip.addr));
    }

    bool isNULL() { return ip.addr == 0; }
   private:
    struct ip_addr ip;

    friend class Messenger;
  };

  Messenger();

  Future<void> Send(NetworkId nid, EbbId id, uint64_t type_code,
                    std::unique_ptr<IOBuf>&& data);
  void Receive(NetworkManager::TcpPcb& t, std::unique_ptr<IOBuf>&& b);

  NetworkId LocalNetworkId();

 private:
  void StartListening(uint16_t port);

  static uint16_t port_;
  NetworkManager::TcpPcb tcp_;
  ebbrt::SpinLock lock_;
  std::unordered_map<uint32_t, SharedFuture<NetworkManager::TcpPcb>>
  connection_map_;
  std::unordered_map<uint32_t, std::unique_ptr<IOBuf>> queued_receives_;

  friend void ebbrt::runtime::Init();
};

constexpr auto messenger = EbbRef<Messenger>(kMessengerId);

}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_MESSENGER_H_
