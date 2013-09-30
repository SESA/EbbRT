/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "arch/args.hpp"
#include "app/app.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "lrt/trans_impl.hpp"
#include "misc/vtable.hpp"
#include "src/lrt/config.hpp"

#ifdef __ebbrt__
ADD_EARLY_CONFIG_SYMBOL(EbbManager, &ebbrt::PrimitiveEbbManagerConstructRoot)
#endif

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol ("EbbManager", ebbrt::PrimitiveEbbManagerConstructRoot);
}

#ifdef __ebbrt__
#include "lrt/bare/assert.hpp"
#endif

namespace {
  void local_cache_rep(ebbrt::EbbId id, ebbrt::EbbRep* rep)
  {
    ebbrt::lrt::trans::cache_rep(id, rep);
  }
}

ebbrt::PrimitiveEbbManager::PrimitiveEbbManager(EbbId id,
                                                std::unordered_map
                                                <EbbId, EbbRoot*>& root_table,
                                                Spinlock& root_table_lock,
                                                std::unordered_map
                                                <EbbId, EbbRoot* (*)()>& factory_table,
                                                Spinlock& factory_table_lock)
  : EbbManager{id}, root_table_(root_table), root_table_lock_(root_table_lock),
    factory_table_(factory_table), factory_table_lock_(factory_table_lock)
{
  next_free_ = lrt::config::get_space_id() << 16 |
    ((1 << 16) /
#ifdef __linux__
     lrt::event::get_max_contexts()
#elif __ebbrt__
     lrt::event::get_num_cores()
#endif
     ) * get_location();
}

void
ebbrt::PrimitiveEbbManager::CacheRep(EbbId id, EbbRep* rep)
{
  local_cache_rep(id, rep);
}

ebbrt::EbbId
ebbrt::PrimitiveEbbManager::AllocateId()
{
  return next_free_++;
}

void
ebbrt::PrimitiveEbbManager::Bind(EbbRoot* (*factory)(), EbbId id)
{
  if ((id >> 16) == ebbrt::lrt::config::get_space_id()) {
    factory_table_lock_.Lock();
    factory_table_[id] = factory;
    factory_table_lock_.Unlock();
  } else {
    //TODO: Go remote
#ifdef __linux__
    assert(0);
#elif __ebbrt__
    LRT_ASSERT(0);
#endif
  }
}

namespace {
  using namespace ebbrt;
  class PrimitiveEbbRoot : public ebbrt::EbbRoot {
  public:
    PrimitiveEbbRoot(std::unordered_map
                     <EbbId, EbbRoot*>& root_table,
                     Spinlock& root_table_lock,
                     std::unordered_map
                     <EbbId, EbbRoot* (*)()>& factory_table,
                     Spinlock& factory_table_lock)
      : root_table_(root_table), root_table_lock_(root_table_lock),
        factory_table_(factory_table), factory_table_lock_(factory_table_lock)
    {
    }
    bool PreCall(Args* args, ptrdiff_t fnum,
                 lrt::trans::FuncRet* fret, EbbId id) override
    {
      EbbRoot* root;
      do {
        root_table_lock_.Lock();
        auto it = root_table_.find(id);
        if (it == root_table_.end()) {
          factory_table_lock_.Lock();
          auto it_fact = factory_table_.find(id);
          if (it_fact == factory_table_.end()) {
            //TODO: Go remote
#ifdef __linux__
            assert(0);
#elif __ebbrt__
            LRT_ASSERT(0);
#endif
          }
          auto factory = it_fact->second;
          factory_table_lock_.Unlock();
          root_table_.insert(std::make_pair(id, nullptr));
          root_table_lock_.Unlock();
          root = factory();
          root_table_lock_.Lock();
          root_table_[id] = root;
        } else {
          root = it->second;
        }
        root_table_lock_.Unlock();
      } while (root == nullptr);
      bool ret = root->PreCall(args, fnum, fret, id);
      if (ret) {
        lrt::event::_event_altstack_push
          (reinterpret_cast<uintptr_t>(root));
      }
      return ret;
    }
    void* PostCall(void* ret) override {
      EbbRoot* root =
        reinterpret_cast<EbbRoot*>
        (lrt::event::_event_altstack_pop());
      return root->PostCall(ret);
    }
  private:
    std::unordered_map<EbbId, EbbRoot*>& root_table_;
    Spinlock& root_table_lock_;
    std::unordered_map<EbbId, EbbRoot* (*)()>& factory_table_;
    Spinlock& factory_table_lock_;
  };
}

void
ebbrt::PrimitiveEbbManager::Install()
{
  root_table_lock_.Lock();
  if (root_table_.empty()) {
    // We need to copy the initial table in, install our new miss
    // handler
    size_t num_init = lrt::config::get_static_ebb_count();
    auto it = root_table_.begin();
    for (unsigned i = 0; i < num_init ; ++i) {
      auto binding = lrt::trans::initial_root_table(i);
      it = root_table_.insert(it, std::make_pair(binding.id,
                                                 binding.root));
    }
    PrimitiveEbbRoot* prim_root = new PrimitiveEbbRoot(root_table_,
                                                       root_table_lock_,
                                                       factory_table_,
                                                       factory_table_lock_);
    lrt::trans::install_miss_handler(prim_root);
  }
  root_table_lock_.Unlock();
}

bool
ebbrt::PrimitiveEbbManagerRoot::PreCall(Args* args,
                                        ptrdiff_t fnum,
                                        lrt::trans::FuncRet* fret,
                                        EbbId id)
{
  // We depend on the memory allocator to construct our reps but the
  // memory allocator can call CacheRep. To prevent deadlock, in the
  // scenario where we fail to acquire the lock and CacheRep was
  // called, we just cache the rep and return without constructing a
  // rep.
  if (!lock_.TryLock()) {
    if (fnum == vtable_index<PrimitiveEbbManager>
        (&PrimitiveEbbManager::CacheRep)) {
      //fetch parameters off from this_pointer location
      EbbId target_id = *((&args->this_pointer()) + 1);
      EbbRep* target_rep =
        *reinterpret_cast<EbbRep**>(((&args->this_pointer()) + 2));
      local_cache_rep(target_id, target_rep);
      return false;
    } else {
      lock_.Lock();
    }
  }
  // We should have the lock by now
  auto it = reps_.find(get_location());
  auto end = reps_.end();
  lock_.Unlock();
  PrimitiveEbbManager* ref;
  bool ret = true;
  if (it == end) {
    // If we missed while trying to cache a rep, we should
    // cache the rep first so that we can allocate memory if
    // need be
    if (fnum == vtable_index<PrimitiveEbbManager>
        (&PrimitiveEbbManager::CacheRep)) {
      //fetch parameters off from this_pointer location
      EbbId target_id = *((&args->this_pointer()) + 1);
      EbbRep* target_rep =
        *reinterpret_cast<EbbRep**>(((&args->this_pointer()) + 2));
      local_cache_rep(target_id, target_rep);
      ret = false;
    }
    lock_.Lock();
    std::unordered_map<EbbId, EbbRoot*>* root_table;
    Spinlock* root_table_lock;
    std::unordered_map<EbbId, EbbRoot* (*)()>* factory_table;
    Spinlock* factory_table_lock;
    if (reps_.empty()) {
      root_table = new std::unordered_map<EbbId, EbbRoot*>;
      root_table_lock = new Spinlock;
      factory_table = new std::unordered_map<EbbId, EbbRoot* (*)()>;
      factory_table_lock = new Spinlock;
    } else {
      root_table = &reps_.begin()->second->root_table_;
      root_table_lock = &reps_.begin()->second->root_table_lock_;
      factory_table = &reps_.begin()->second->factory_table_;
      factory_table_lock = &reps_.begin()->second->factory_table_lock_;
    }
    ref = new PrimitiveEbbManager(id, *root_table, *root_table_lock,
                                  *factory_table, *factory_table_lock);
    local_cache_rep(id, ref);
    reps_[get_location()] = ref;
    lock_.Unlock();
  } else {
    local_cache_rep(id, it->second);
    ref = it->second;
  }

  args->this_pointer() = reinterpret_cast<uintptr_t>(ref);
  // rep is a pointer to pointer to array 256 of pointer to
  // function returning void
  void (*(**rep)[256])() = reinterpret_cast<void (*(**)[256])()>(ref);
  fret->func = (**rep)[fnum];
  return ret;
}


void*
ebbrt::PrimitiveEbbManagerRoot::PostCall(void* ret)
{
  return ret;
}

ebbrt::EbbRoot* ebbrt::PrimitiveEbbManagerConstructRoot()
{
  static PrimitiveEbbManagerRoot root;
  return &root;
}
