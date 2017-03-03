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
#include "../GlobalIdMapBase.h"
#include "../Message.h"
#include "../StaticSharedEbb.h"
#include "EbbRef.h"
#include "Runtime.h"
#include "StaticIds.h"

#pragma weak InstallGlobalIdMap

namespace ebbrt {

void InstallGlobalIdMap(); 

class DefaultGlobalIdMap : public GlobalIdMap,
                    public CacheAligned,
                    public Messagable<DefaultGlobalIdMap> {
 public:

  DefaultGlobalIdMap();
  static EbbRef<DefaultGlobalIdMap> Create(EbbId id){
    auto base = new DefaultGlobalIdMap::Base();
    local_id_map->Insert(std::make_pair(id, static_cast<GlobalIdMap::Base *>(base)));
    return EbbRef<DefaultGlobalIdMap>(id);
  }
  static DefaultGlobalIdMap &HandleFault(EbbId id){
    return static_cast<DefaultGlobalIdMap &>(GlobalIdMap::HandleFault(id));
  }
  class Base : public GlobalIdMap::Base {
  public:
    GlobalIdMap &Construct(EbbId id) override {
      // shared root
      if (constructed_){
        return *rep_;
      }
      rep_ = new DefaultGlobalIdMap();
      constructed_ = true;
      // Cache the reference to the rep in the local translation table
      EbbRef<DefaultGlobalIdMap>::CacheRef(id, *rep_);
      return *rep_;
    }
  private:
    DefaultGlobalIdMap* rep_;
    bool constructed_ = false;
  };

  Future<std::string> Get(EbbId id, std::string path = std::string()) override;
  Future<void> Set(EbbId id, std::string data, std::string path = std::string()) override;

  void ReceiveMessage(Messenger::NetworkId nid, std::unique_ptr<IOBuf>&& buf);

  static void SetAddress(uint32_t addr);

 private:
  ebbrt::SpinLock lock_;
  std::unordered_map<uint64_t, Promise<std::string>> map_;
  uint64_t val_;
};

constexpr auto global_id_map = EbbRef<DefaultGlobalIdMap>(kGlobalIdMapId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_
