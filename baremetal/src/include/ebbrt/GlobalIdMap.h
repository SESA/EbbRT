//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_

#include <string>

#include <capnp/message.h>

#include <ebbrt/CacheAligned.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/Future.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/Net.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>
#include <ebbrt/Runtime.h>

namespace ebbrt {
class GlobalIdMap : public StaticSharedEbb<GlobalIdMap>, public CacheAligned {
 public:
  GlobalIdMap();

  Future<std::string> Get(EbbId id);

  void HandleMessage(Messenger::NetworkId nid, Buffer buf);

 private:
  static void SetAddress(uint32_t addr);

  NetworkManager::TcpPcb tcp_;
  ebbrt::SpinLock lock_;
  std::unordered_map<uint64_t, Promise<std::string>> map_;
  uint64_t val_;

  friend void ebbrt::runtime::Init();
};

constexpr auto global_id_map = EbbRef<GlobalIdMap>(kGlobalIdMapId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_
