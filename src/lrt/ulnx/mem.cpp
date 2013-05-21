#include <cstdio>
#include <cstdint>

#include "lrt/mem.hpp"


bool
ebbrt::lrt::mem::init(unsigned num_cores)
{
  std::printf("mem init\n");
  return 1;
}

void*
ebbrt::lrt::mem::malloc(size_t size, event::Location loc)
{
  return NULL;
}

void*
ebbrt::lrt::mem::memalign(size_t boundary, size_t size, event::Location loc)
{
  /* kludge */
  return NULL;
}

