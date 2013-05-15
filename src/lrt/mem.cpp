#include "lrt/mem_impl.hpp"

extern char kend[];
char* ebbrt::lrt::mem::mem_start = kend;

ebbrt::lrt::mem::Region* ebbrt::lrt::mem::regions;

void*
ebbrt::lrt::mem::malloc(size_t size, event::Location loc)
{
  return memalign(8, size, loc);
}

void*
ebbrt::lrt::mem::memalign(size_t boundary, size_t size, event::Location loc)
{
  Region* r = &regions[loc];
  char* p = r->current;
  p = reinterpret_cast<char*>(((reinterpret_cast<uintptr_t>(p) + boundary - 1)
                               / boundary) * boundary);
  if ((p + size) > r->end) {
    return nullptr;
  }
  r->current = p + size;
  return p;
}
