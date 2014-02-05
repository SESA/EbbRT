//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_GLOBALMAP_H_
#define HOSTED_SRC_INCLUDE_EBBRT_GLOBALMAP_H_

#include <utility>

#include <boost/asio.hpp>

#include <ebbrt/EbbRef.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {
class GlobalMap : public StaticSharedEbb<GlobalMap> {
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket socket_;

  class session : public std::enable_shared_from_this<session> {
   public:
    explicit session(boost::asio::ip::tcp::socket socket);

   private:
    boost::asio::ip::tcp::socket socket_;
  };
  friend class session;

  void DoAccept();
  uint16_t GetPort();
  friend class NodeAllocator;

 public:
  GlobalMap();
};
const constexpr auto global_map = EbbRef<GlobalMap>(kGlobalMapId);
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_GLOBALMAP_H_
