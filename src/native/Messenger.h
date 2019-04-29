//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_MESSENGER_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_MESSENGER_H_

#include <string>

#include "../CacheAligned.h"
#include "../Future.h"
#include "../SpinLock.h"
#include "../StaticSharedEbb.h"
#include "NetTcpHandler.h"
#include "Runtime.h"
#include "StaticIds.h"

namespace ebbrt {

class Messenger : public StaticSharedEbb<Messenger>, public CacheAligned {
 public:
  struct Header {
    uint64_t length;
    uint64_t type_code;
    EbbId id;
  };

  class NetworkId {
   public:
    explicit NetworkId(Ipv4Address ip_) : ip(ip_) {}
    explicit NetworkId(uint32_t ip_h) { ip = htonl(ip_h); }
    explicit NetworkId(std::string str) {
      if (str.size() != 4)
        throw std::runtime_error(
            "Cannot build NetworkId from string, size mismatch");

      ip = Ipv4Address(*reinterpret_cast<const uint32_t*>(str.data()));
    }
    bool operator==(const NetworkId& rhs) { return ip == rhs.ip; }

    static NetworkId FromBytes(const unsigned char* p, size_t len) {
      assert(len == 4);
      return NetworkId(ntohl(*reinterpret_cast<const uint32_t*>(p)));
    }

    std::string ToBytes() {
      return std::string(reinterpret_cast<const char*>(ip.toArray().data()), 4);
    }

    std::string ToString() {
      auto data = ip.toArray().data();
      if (ip.toArray().size() != 4) {
        throw std::runtime_error("ebbrt::Messenger::NetworkId.ToString()-"
                                 "NetworkId size not equal to 4");
      }
      char ss[18];
      std::sprintf(ss, "%u.%u.%u.%u", data[0], data[1], data[2], data[3]);
      return std::string(ss);
    }

   private:
    Ipv4Address ip;

    friend class Messenger;
  };

  static void ClassInit() {} // no class wide static initialization logic
  
  Messenger();

  Future<void> Send(NetworkId nid, EbbId id, uint64_t type_code,
                    std::unique_ptr<IOBuf>&& data);
  void Receive(NetworkManager::TcpPcb& t, std::unique_ptr<IOBuf>&& b);

  NetworkId LocalNetworkId();
  uint16_t GetPort();

  void StartListening(uint16_t port);

 private:
  class Connection : public TcpHandler {
   public:
    explicit Connection(NetworkManager::TcpPcb pcb);

    void Receive(std::unique_ptr<MutIOBuf> b) override;
    void Connected() override;
    void Close() override;
    void Abort() override;
    Future<Connection*> GetFuture();

   private:
    static const constexpr double kOccupancyRatio = 0.20;
    static const constexpr uint8_t kPreallocateChainLen = 100;
    void check_preallocate();
    void preallocated(std::unique_ptr<MutIOBuf> buf);
    void process_message(std::unique_ptr<MutIOBuf> b);
    std::unique_ptr<MutIOBuf>
    process_message_chain(std::unique_ptr<MutIOBuf> b);

    uint32_t preallocate_{0};
    std::unique_ptr<ebbrt::MutIOBuf> buf_;
    ebbrt::Promise<Connection*> promise_;
  };

  static uint16_t port_;
  NetworkManager::ListeningTcpPcb listening_pcb_;
  ebbrt::SpinLock lock_;
  std::unordered_map<Ipv4Address, SharedFuture<Connection*>> connection_map_;
};

constexpr auto messenger = EbbRef<Messenger>(kMessengerId);

}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_MESSENGER_H_
