//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_

#include <string>

#include <capnp/message.h>

#include "../CacheAligned.h"
#include "../Future.h"
#include "../Message.h"
#include "../StaticSharedEbb.h"
#include "EbbRef.h"
#include "Runtime.h"
#include "StaticIds.h"

namespace ebbrt {
class GlobalIdMap : public StaticSharedEbb<GlobalIdMap>,
                    public CacheAligned,
                    public Messagable<GlobalIdMap> {
 public:
  GlobalIdMap();

  Future<std::string> Get(EbbId id);
  Future<void> Set(EbbId id, std::string data);

  void ReceiveMessage(Messenger::NetworkId nid, std::unique_ptr<IOBuf>&& buf);

  static void SetAddress(uint32_t addr);

 private:
  ebbrt::SpinLock lock_;
  std::unordered_map<uint64_t, Promise<std::string>> map_;
  uint64_t val_;
};

constexpr auto global_id_map = EbbRef<GlobalIdMap>(kGlobalIdMapId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_
