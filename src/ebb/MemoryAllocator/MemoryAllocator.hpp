#ifndef EBBRT_EBB_MEMORYALLOCATOR_MEMORYALLOCATOR_HPP
#define EBBRT_EBB_MEMORYALLOCATOR_MEMORYALLOCATOR_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class MemoryAllocator : public EbbRep {
  public:
    virtual void* malloc(size_t size) = 0;
    virtual void* memalign(size_t boundary, size_t size) = 0;
    virtual void free(void* ptr) = 0;
    virtual void* realloc(void* ptr, size_t size) = 0;
    virtual void* calloc(size_t num, size_t size) = 0;
    virtual ~MemoryAllocator() {}
  };
  extern char mem_allocator_id_resv
  __attribute__ ((section ("static_ebb_ids")));
  extern "C" char static_ebb_ids_start[];
  const Ebb<MemoryAllocator> memory_allocator =
    Ebb<MemoryAllocator>(static_cast<EbbId>(&mem_allocator_id_resv -
                                            static_ebb_ids_start));
}

#endif
