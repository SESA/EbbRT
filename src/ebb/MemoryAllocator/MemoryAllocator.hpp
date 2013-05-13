#ifndef EBBRT_EBB_MEMORYALLOCATOR_MEMORYALLOCATOR_HPP
#define EBBRT_EBB_MEMORYALLOCATOR_MEMORYALLOCATOR_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class MemoryAllocator : public EbbRep {
  public:
    virtual void* malloc(size_t size) = 0;
    virtual void* memalign(size_t boundary, size_t size) = 0;
    virtual void free(void* ptr) = 0;
  };
  extern Ebb<MemoryAllocator> memory_allocator;
}

#endif
