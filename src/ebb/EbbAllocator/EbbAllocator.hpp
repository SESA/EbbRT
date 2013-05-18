#ifndef EBBRT_EBB_EBBALLOCATOR_EBBALLOCATOR_HPP
#define EBBRT_EBB_EBBALLOCATOR_EBBALLOCATOR_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class EbbAllocator : public EbbRep {
  public:
    virtual void CacheRep(EbbId id, EbbRep* rep) = 0;
    virtual ~EbbAllocator() {}
  };
  extern char ebb_allocator_id_resv
  __attribute__ ((section ("static_ebb_ids")));
  extern "C" char static_ebb_ids_start[];
  const Ebb<EbbAllocator> ebb_allocator =
    Ebb<EbbAllocator>(static_cast<EbbId>(&ebb_allocator_id_resv -
                                         static_ebb_ids_start));
}

#endif
