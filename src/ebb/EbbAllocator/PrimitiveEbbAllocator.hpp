#ifndef EBBRT_EBB_EBBALLOCATOR_PRIMITIVEEBBALLOCATOR_HPP
#define EBBRT_EBB_EBBALLOCATOR_PRIMITIVEEBBALLOCATOR_HPP

#include <map>

#include "ebb/EbbAllocator/EbbAllocator.hpp"

namespace ebbrt {
  class PrimitiveEbbAllocator : public EbbAllocator {
  public:
    void CacheRep(EbbId id, EbbRep* rep) override;
  };

  extern "C" EbbRoot* PrimitiveEbbAllocatorConstructRoot();
  class PrimitiveEbbAllocatorRoot : public EbbRoot {
  public:
    bool PreCall(Args* args, ptrdiff_t fnum,
                 lrt::trans::FuncRet* fret, EbbId id) override;
    void* PostCall(void* ret) override;
  private:
    std::map<Location, PrimitiveEbbAllocator*> reps_;
  };
}

#endif
