#ifndef EBBRT_EBB_EBBMANAGER_PRIMITIVEEBBMANAGER_HPP
#define EBBRT_EBB_EBBMANAGER_PRIMITIVEEBBMANAGER_HPP

#include <map>

#include "ebb/EbbManager/EbbManager.hpp"

namespace ebbrt {
  class PrimitiveEbbManager : public EbbManager {
  public:
    void CacheRep(EbbId id, EbbRep* rep) override;
  };

  EbbRoot* PrimitiveEbbManagerConstructRoot();
  class PrimitiveEbbManagerRoot : public EbbRoot {
  public:
    bool PreCall(Args* args, ptrdiff_t fnum,
                 lrt::trans::FuncRet* fret, EbbId id) override;
    void* PostCall(void* ret) override;
  private:
    std::map<Location, PrimitiveEbbManager*> reps_;
  };
}

#endif
