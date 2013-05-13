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
    static void* operator new(size_t size);
    static void* operator new(size_t size, void* ptr);
    static void operator delete(void* p);
  private:
    char* start_;
    char* current_;
    char* end_;
  };

  extern "C" EbbRoot* SimpleMemoryAllocatorConstructRoot();
  class SimpleMemoryAllocatorRoot : public EbbRoot {
  public:
    bool PreCall(Args* args, ptrdiff_t fnum, lrt::trans::FuncRet* fret) override;
    void* PostCall(void* ret) override;
  private:
    friend EbbRoot* SimpleMemoryAllocatorConstructRoot();
    std::unordered_map<Location, SimpleMemoryAllocator*> reps_;
  };
}

#endif
