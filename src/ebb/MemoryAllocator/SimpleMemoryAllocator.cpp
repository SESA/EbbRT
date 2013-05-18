#include <cstring>
#include <new>

#include "ebb/DistributedRoot.hpp"
#include "ebb/EbbAllocator/EbbAllocator.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "lrt/mem_impl.hpp"
#include "lrt/trans_impl.hpp"

using namespace ebbrt;

ebbrt::SimpleMemoryAllocator::SimpleMemoryAllocator(Location loc)
{
  lrt::mem::Region* r = &lrt::mem::regions[loc];
  start_ = r->start;
  current_ = r->current;
  end_ = r->end;
  r->start = r->end;
}

void*
ebbrt::SimpleMemoryAllocator::malloc(size_t size)
{
  return memalign(8, size);
}

void*
ebbrt::SimpleMemoryAllocator::memalign(size_t boundary, size_t size) {
  char* p = current_;
  p = reinterpret_cast<char*>(((reinterpret_cast<uintptr_t>(p) +
                                boundary - 1) / boundary) * boundary);

  if ((p + size) > end_) {
    return nullptr;
  }
  current_ = p + size;
  return p;
}

void
ebbrt::SimpleMemoryAllocator::free(void* p)
{
}

void*
ebbrt::SimpleMemoryAllocator::realloc(void* ptr, size_t size)
{
  free(ptr);
  return malloc(size);
}

void*
ebbrt::SimpleMemoryAllocator::calloc(size_t num, size_t size)
{
  size_t total = num * size;
  void* ptr = malloc(total);
  return std::memset(ptr, 0, total);
}

void*
ebbrt::SimpleMemoryAllocator::operator new(size_t size)
{
  return lrt::mem::malloc(size, get_location());
}

void
ebbrt::SimpleMemoryAllocator::operator delete(void* p)
{
}

ebbrt::EbbRoot* ebbrt::SimpleMemoryAllocatorConstructRoot()
{
  static DistributedRoot<SimpleMemoryAllocator> root;
  return &root;
}
