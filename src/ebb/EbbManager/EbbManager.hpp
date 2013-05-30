#ifndef EBBRT_EBB_EBBMANAGER_EBBMANAGER_HPP
#define EBBRT_EBB_EBBMANAGER_EBBMANAGER_HPP

#include "ebb/ebb.hpp"
#include "lrt/trans_impl.hpp"

namespace ebbrt {
  class EbbManager : public EbbRep {
  public:
    virtual void CacheRep(EbbId id, EbbRep* rep) = 0;
    virtual EbbId AllocateId() = 0;
    virtual void Bind(EbbRoot* (*factory)(), EbbId id) = 0;
    virtual ~EbbManager() {}
  private:
    friend lrt::trans::InitRoot;
    virtual void Install() = 0;
  };
  const EbbRef<EbbManager> ebb_manager =
    EbbRef<EbbManager>(lrt::trans::find_static_ebb_id("EbbManager"));
}

#endif
