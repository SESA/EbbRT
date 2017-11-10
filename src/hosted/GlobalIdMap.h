//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_
#define HOSTED_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_

#include <string>
#include <utility>

#include "../Debug.h"
#include "../EbbAllocator.h"
#include "../EbbRef.h"
#include "../Future.h"
#include "../GlobalIdMapBase.h"
#include "../Message.h"
#include "../StaticIds.h"
#include "../StaticSharedEbb.h"

#pragma weak InstallGlobalIdMap

namespace ebbrt {

void InstallGlobalIdMap();

namespace {

}

class DefaultGlobalIdMap : public GlobalIdMap,
                           public CacheAligned,
                           public Messagable<DefaultGlobalIdMap> {
 public:
  DefaultGlobalIdMap();
  static EbbRef<DefaultGlobalIdMap>
  Create(EbbId id = ebb_allocator->Allocate()) {
    auto root = new DefaultGlobalIdMap::Base();
    local_id_map->Insert(
        std::make_pair(id, static_cast<GlobalIdMap::Base*>(root)));
    return EbbRef<DefaultGlobalIdMap>(id);
  }
  static DefaultGlobalIdMap& HandleFault(EbbId id) {
    return static_cast<DefaultGlobalIdMap&>(GlobalIdMap::HandleFault(id));
  }
  class Base : public GlobalIdMap::Base {
   public:
    GlobalIdMap& Construct(EbbId id) override {
      auto rep = new DefaultGlobalIdMap();
      // Cache the reference to the rep in the local translation table
      EbbRef<DefaultGlobalIdMap>::CacheRef(id, *rep);
      return *rep;
    }
  };
  Future<std::string> Get(EbbId id, const OptArgs& args = OptArgs()) override;
  Future<void> Set(EbbId id, const OptArgs& args = OptArgs()) override;

  void ReceiveMessage(Messenger::NetworkId nid, std::unique_ptr<IOBuf>&& buf);

 private:
  std::mutex m_;
  std::unordered_map<EbbId, std::string> map_;
};

const constexpr auto global_id_map = EbbRef<DefaultGlobalIdMap>(kGlobalIdMapId);
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_
