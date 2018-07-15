//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef EBBRT_ZKGLOBALIDMAP_H_
#define EBBRT_ZKGLOBALIDMAP_H_

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include <ebbrt/CacheAligned.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/Future.h>
#include <ebbrt/GlobalIdMapBase.h>
#include <ebbrt/Message.h>
#include <ebbrt/StaticIds.h>

#include "ZooKeeper.h"

namespace ebbrt {

void InstallGlobalIdMap();

class ZKGlobalIdMap : public GlobalIdMap, public CacheAligned {
 public:
  struct ZKOptArgs : public GlobalIdMap::OptArgs {
    std::string path = std::string();
    ZooKeeper::Flag flags = ZooKeeper::Flag::Nil;
  };

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
    ebbrt::kprintf("ZKGlobalIdMap init called\n");
    zk_ =
        ebbrt::ZooKeeper::Create(ebbrt::ebb_allocator->Allocate(), &zkwatcher_);
    return zkwatcher_.connected_.GetFuture();
  }

  Future<std::vector<std::string>> List(EbbId id, const ZKOptArgs& args) {
    std::string path = args.path;
    auto p = new ebbrt::Promise<std::vector<std::string>>;
    auto ret = p->GetFuture();
    auto fullpath = get_id_path(id, path);
    zk_->GetChildren(std::string(fullpath)).Then([p](auto f) {
      auto zn_children = f.Get();
      p->SetValue(zn_children.values);
    });
    return ret;
  }

  Future<std::string> Get(EbbId id, const GlobalIdMap::OptArgs& args) override {
    auto zkargs = static_cast<const ZKOptArgs&>(args);
    std::string path = zkargs.path;
    auto p = new ebbrt::Promise<std::string>;
    auto f = p->GetFuture();
    auto fullpath = get_id_path(id, path);
    zk_->Get(std::string(fullpath))
        .Then([p](ebbrt::Future<ebbrt::ZooKeeper::Znode> z) {
          auto znode = z.Get();
          p->SetValue(znode.value);
        });
    return f;
  }

  Future<bool> Exists(EbbId id, const ZKOptArgs& args) {
    std::string path = args.path;
    auto fullpath = get_id_path(id, path);
    return zk_->Exists(fullpath);
  }

  Future<void> Set(EbbId id, const GlobalIdMap::OptArgs& args) override {
    auto zkargs = static_cast<const ZKOptArgs&>(args);
    auto val = zkargs.data;
    auto path = zkargs.path;
    auto p = new ebbrt::Promise<void>;
    auto ret = p->GetFuture();
    std::cout << "ZKM Set Called for: " << id << std::endl;
    auto fullpath = get_id_path(id, path);
    std::cout << "ZKM path=" << id << " " << fullpath << std::endl;
        std::cout << "Setting Value " << fullpath << " = " << val << std::endl;
        zk_->Set(fullpath, val).Then([p](auto f) { p->SetValue(); });
    return ret;
  }

  void SetWatcher(EbbId id, Watcher* w, std::string path = std::string()) {
    auto fullpath = get_id_path(id, path);
    zk_->Stat(fullpath, w);
    zk_->GetChildren(fullpath, w);
    return;
  }

 private:
  std::string get_id_path(EbbId id, std::string path = std::string()) {
    std::string fullpath;
    auto hit = ebbid_cache_.find(id);
    if (hit == ebbid_cache_.end()) { /* ebbid cache miss */
      std::ostringstream s;
      s << '/' << id;
      fullpath = s.str();
      ebbid_cache_[id] = fullpath;
    } else { /* ebbid cache hit */
      fullpath = hit->second;
    }
    if (!path.empty()) {
      fullpath += "/" + path;
    }
    return fullpath;
  }

  struct ConnectionWatcher : ebbrt::ZooKeeper::ConnectionWatcher {
    void OnConnected() override {
      ebbrt::kprintf("ZKGlobalIdMap: ZooKeeper session established.\n");
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
  std::unordered_map<ebbrt::EbbId, std::string> ebbid_cache_;
  ConnectionWatcher zkwatcher_;
  ebbrt::EbbRef<ebbrt::ZooKeeper> zk_;
};

constexpr auto zkglobal_id_map = EbbRef<ZKGlobalIdMap>(kGlobalIdMapId);

}  // namespace ebbrt
#endif  // EBBRT_ZKGLOBALIDMAP_H_
