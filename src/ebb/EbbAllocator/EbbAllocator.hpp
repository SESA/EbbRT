#ifndef EBBRT_EBB_EBBALLOCATOR_EBBALLOCATOR_HPP
#define EBBRT_EBB_EBBALLOCATOR_EBBALLOCATOR_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class EbbAllocator : public EbbRep {
  public:
    virtual void CacheRep(EbbId id, EbbRep* rep) = 0;
  };
  extern Ebb<EbbAllocator> ebb_allocator;
}

#endif
