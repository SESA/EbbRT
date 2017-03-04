//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef EBBRT_ZKGLOBALIDMAP_H_
#define EBBRT_ZKGLOBALIDMAP_H_

#include <string>

#include <ebbrt/CacheAligned.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/Future.h>
#include <ebbrt/GlobalIdMapBase.h>
#include <ebbrt/Message.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/SharedEbb.h>
#include <ebbrt/StaticIds.h>

#include "ZooKeeper.h"

namespace ebbrt {

class ZKGlobalIdMap : public GlobalIdMap, public CacheAligned {
 public:
  static EbbRef<ZKGlobalIdMap> Create(EbbId id) {
    auto base = new ZKGlobalIdMap::Base();
    local_id_map->Insert(
        std::make_pair(id, static_cast<GlobalIdMap::Base*>(base)));
    return EbbRef<ZKGlobalIdMap>(id);
  }
  static ZKGlobalIdMap& HandleFault(EbbId id) {
    return static_cast<ZKGlobalIdMap&>(GlobalIdMap::HandleFault(id));
  }
  class Base : public GlobalIdMap::Base {
   public:
    GlobalIdMap& Construct(EbbId id) override {
      // shared root
      if (constructed_) {
        return *rep_;
      }
      rep_ = new ZKGlobalIdMap();
      constructed_ = true;
      // Cache the reference to the rep in the local translation table
      EbbRef<ZKGlobalIdMap>::CacheRef(id, *rep_);
      return *rep_;
    }

   private:
    ZKGlobalIdMap* rep_;
    bool constructed_ = false;
  };
  typedef ebbrt::ZooKeeper::Watcher Watcher;
  typedef ebbrt::ZooKeeper::WatchEvent WatchEvent;

  ZKGlobalIdMap(){};
  Future<bool> Init() {
    zk_ =
        ebbrt::ZooKeeper::Create(ebbrt::ebb_allocator->Allocate(), &zkwatcher_);
    return zkwatcher_.connected_.GetFuture();
  }
  Future<std::vector<std::string>> List(EbbId id,
                                        std::string path = std::string()) {
    auto p = new ebbrt::Promise<std::vector<std::string>>;
    auto ret = p->GetFuture();
    char buff[100];
    sprintf(buff, "/%d", id);
    zk_->GetChildren(std::string(buff)).Then([p](auto f) {
      auto zn_children = f.Get();
      p->SetValue(zn_children.values);
    });
    return ret;
  }

  Future<std::string> Get(EbbId id, std::string path = std::string()) override {
    char buff[100];
    sprintf(buff, "/%d", id);
    auto p = new ebbrt::Promise<std::string>;
    auto f = p->GetFuture();
    zk_->Get(std::string(buff))
        .Then([p](ebbrt::Future<ebbrt::ZooKeeper::Znode> z) {
          auto znode = z.Get();
          p->SetValue(znode.value);
        });
    return f;
  }

  Future<void> Set(EbbId id, std::string val,
                   std::string path = std::string()) override {
    auto p = new ebbrt::Promise<void>;
    auto ret = p->GetFuture();
    char buff[100];
    sprintf(buff, "/%d", id);
    zk_->Exists(std::string(buff)).Then([this, p, buff, val](auto b) {
      if (b.Get() == true) {
        zk_->Set(std::string(buff), val).Then([p](auto f) { p->SetValue(); });
      } else {
        zk_->New(std::string(buff), val).Then([p](auto f) { p->SetValue(); });
      }
    });
    return ret;
  }

  void SetWatcher(EbbId id, Watcher* w, std::string path = std::string()) {
    char buf[15];
    sprintf(buf, "/%d", id);
    zk_->Stat(std::string(buf), w).Block();
    return;
  }

 private:
  struct ConnectionWatcher : ebbrt::ZooKeeper::ConnectionWatcher {
    void OnConnected() override {
      ebbrt::kprintf("GlobalIdMap: ZooKeeper session established.\n");
      connected_.SetValue(true);
    }
    void OnConnecting() override {
      ebbrt::kprintf("GlobalIdMap: ZooKeeper session connecting...\n");
    }
    void OnSessionExpired() override {
      connected_.SetValue(false);
      ebbrt::kprintf("GlobalIdMap: ZooKeeper session expired.\n");
    }
    void OnAuthFailed() override {
      connected_.SetValue(false);
      ebbrt::kprintf("GlobalIdMap: ZooKeeper session failed.\n");
    }
    ebbrt::Promise<bool> connected_;
  };
  ConnectionWatcher zkwatcher_;
  ebbrt::EbbRef<ebbrt::ZooKeeper> zk_;
};

void InstallGlobalIdMap() {
  ebbrt::kprintf("Installing ZooKeeper GlobalIdMap\n");
  ebbrt::ZKGlobalIdMap::Create(ebbrt::kGlobalIdMapId);
}

constexpr auto zkglobal_id_map = EbbRef<ZKGlobalIdMap>(kGlobalIdMapId);
// EbbRef<GlobalIdMap> global_id_map = EbbRef<GlobalIdMap>(kGlobalIdMapId);
}  // namespace ebbrt
#endif  // EBBRT_ZKGLOBALIDMAP_H_
