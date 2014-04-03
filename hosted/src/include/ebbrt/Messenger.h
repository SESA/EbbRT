//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_MESSENGER_H_
#define HOSTED_SRC_INCLUDE_EBBRT_MESSENGER_H_

#include <mutex>

#include <boost/asio.hpp>

#include <ebbrt/Buffer.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/Future.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {
class Messenger : public StaticSharedEbb<Messenger> {
 public:
  class NetworkId {
   public:
    explicit NetworkId(boost::asio::ip::address_v4 ip) : ip_(std::move(ip)) {}

    std::string ToBytes() {
      auto a = ip_.to_bytes();
      return std::string(reinterpret_cast<char*>(a.data()), a.size());
    }

    std::string ToString() { return ip_.to_string(); }

    bool operator==(const NetworkId &rhs) {
      return ip_ == rhs.ip_;
    }

   private:
    boost::asio::ip::address_v4 ip_;

    friend class Messenger;
  };

  Messenger();

  Future<void> Send(NetworkId to, EbbId id, uint64_t type_code,
                    std::unique_ptr<const IOBuf>&& data);
  NetworkId LocalNetworkId();

 private:
  struct Header {
    uint64_t length;
    uint64_t type_code;
    EbbId id;
  };

  class Session : public std::enable_shared_from_this<Session> {
   public:
    explicit Session(boost::asio::ip::tcp::socket socket);

    void Start();
    Future<void> Send(std::unique_ptr<const IOBuf>&& data);

   private:
    void ReadHeader();
    void ReadMessage();

    Header header_;
    boost::asio::ip::tcp::socket socket_;
  };

  void DoAccept(std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor,
                std::shared_ptr<boost::asio::ip::tcp::socket> socket);
  uint16_t GetPort();

  uint16_t port_;
  std::mutex m_;
  std::unordered_map<uint32_t, SharedFuture<std::weak_ptr<Session>>>
  connection_map_;

  friend class Session;
  friend class NodeAllocator;
};

const constexpr auto messenger = EbbRef<Messenger>(kMessengerId);
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_MESSENGER_H_
