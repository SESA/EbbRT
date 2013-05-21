#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "lrt/trans_impl.hpp"
#include "misc/vtable.hpp"

namespace {
  void local_cache_rep(ebbrt::EbbId id, ebbrt::EbbRep* rep)
  {
    ebbrt::lrt::trans::cache_rep(id, rep);
  }
}

void
ebbrt::PrimitiveEbbManager::CacheRep(EbbId id, EbbRep* rep)
{
  local_cache_rep(id, rep);
}

bool
ebbrt::PrimitiveEbbManagerRoot::PreCall(Args* args,
                                          ptrdiff_t fnum,
                                          lrt::trans::FuncRet* fret,
                                          EbbId id)
{
  auto it = reps_.find(get_location());
  PrimitiveEbbManager* ref;
  bool ret = true;
  if (it == reps_.end()) {
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
    ref = new PrimitiveEbbManager();
    local_cache_rep(id, ref);
    reps_[get_location()] = ref;
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
