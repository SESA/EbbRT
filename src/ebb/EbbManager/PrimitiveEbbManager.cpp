#include "app/app.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "lrt/trans_impl.hpp"
#include "misc/vtable.hpp"

namespace {
  void local_cache_rep(ebbrt::EbbId id, ebbrt::EbbRep* rep)
  {
    ebbrt::lrt::trans::cache_rep(id, rep);
  }
}

ebbrt::PrimitiveEbbManager::PrimitiveEbbManager(std::unordered_map
                                                <EbbId, EbbRoot*>& root_table,
                                                Spinlock& root_table_lock,
                                                std::unordered_map
                                                <EbbId, EbbRoot* (*)()>& factory_table,
                                                Spinlock& factory_table_lock)
  : root_table_(root_table), root_table_lock_(root_table_lock),
    factory_table_(factory_table), factory_table_lock_(factory_table_lock)
{
  next_free_ = app::config.node_id << 16 |
    ((1 << 16) / lrt::event::get_num_cores()) * get_location();
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
  if ((id >> 16) == app::config.node_id) {
    factory_table_lock_.Lock();
    factory_table_[id] = factory;
    factory_table_lock_.Unlock();
  } else {
    //TODO: Go remote
    while (1)
      ;
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
      root_table_lock_.Lock();
      auto it = root_table_.find(id);
      EbbRoot* root;
      if (it == root_table_.end()) {
        factory_table_lock_.Lock();
        auto it_fact = factory_table_.find(id);
        if (it_fact == factory_table_.end()) {
          //TODO: Go remote
          while (1)
            ;
        }
        factory_table_lock_.Unlock();
        root = it_fact->second();
        root_table_.insert(std::make_pair(id, root));
      } else {
        root = it->second;
      }
      root_table_lock_.Unlock();
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
    auto it = root_table_.begin();
    for (unsigned i = 0; i < app::config.num_init; ++i) {
      it = root_table_.insert(it, std::make_pair(lrt::trans::initial_root_table[i].id,
                                                 lrt::trans::initial_root_table[i].root));
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
  lock_.Lock();
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
      //FIXME: not portable
      EbbId target_id = args->rsi;
      EbbRep* target_rep = reinterpret_cast<EbbRep*>(args->rdx);
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
    ref = new PrimitiveEbbManager(*root_table, *root_table_lock,
                                  *factory_table, *factory_table_lock);
    local_cache_rep(id, ref);
    reps_[get_location()] = ref;
    lock_.Unlock();
  } else {
    local_cache_rep(id, it->second);
    ref = it->second;
  }
  *reinterpret_cast<EbbRep**>(args) = ref;
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
