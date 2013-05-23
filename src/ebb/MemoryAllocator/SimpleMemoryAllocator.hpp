#ifndef EBBRT_EBB_MEMORYALLOCATOR_SIMPLEMEMORYALLOCATOR_HPP
#define EBBRT_EBB_MEMORYALLOCATOR_SIMPLEMEMORYALLOCATOR_HPP

#include <unordered_map>

#include "ebb/MemoryAllocator/MemoryAllocator.hpp"


namespace ebbrt {
  class SimpleMemoryAllocator : public MemoryAllocator {
  public:
    SimpleMemoryAllocator(Location loc = get_location());
    void* malloc(size_t size) override;
    void* memalign(size_t boundary, size_t size) override;
    void free(void* ptr) override;
    void* realloc(void* ptr, size_t size) override;
    void* calloc(size_t num, size_t size) override;
    static void* operator new(size_t size);
    static void operator delete(void* p);
  private:
    char* start_;
    char* current_;
    char* end_;
  };

  EbbRoot* SimpleMemoryAllocatorConstructRoot();
}

#endif
