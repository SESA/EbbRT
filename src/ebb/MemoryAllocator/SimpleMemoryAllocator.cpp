#include <new>

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
ebbrt::SimpleMemoryAllocator::operator new(size_t size)
{
  return lrt::mem::malloc(size, 0);
}

extern "C"
ebbrt::EbbRoot* ebbrt::SimpleMemoryAllocatorConstructRoot()
{
  auto root = new (lrt::mem::malloc(sizeof(SimpleMemoryAllocatorRoot), 0))
    SimpleMemoryAllocatorRoot;
  return root;
}

bool
ebbrt::SimpleMemoryAllocatorRoot::PreCall(Args* args, ptrdiff_t fnum,
                                          lrt::trans::FuncRet* fret)
{
  auto it = reps_.find(get_location());
  SimpleMemoryAllocator* ref;
  if (it == reps_.end()) {
    // No rep for this location
    ref = new SimpleMemoryAllocator();
    ebb_allocator->CacheRep(memory_allocator, ref);
    reps_[get_location()] = ref;
  } else {
    ebb_allocator->CacheRep(memory_allocator, it->second);
    ref = it->second;
  }
  *reinterpret_cast<EbbRep**>(args) = ref;
  // rep is a pointer to pointer to array 256 of pointer to
  // function returning void
  void (*(**rep)[256])() = reinterpret_cast<void (*(**)[256])()>(ref);
  fret->func = (**rep)[fnum];
  return true;
}

void*
ebbrt::SimpleMemoryAllocatorRoot::PostCall(void* ret)
{
  return ret;
}
