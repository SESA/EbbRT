#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "lrt/mem.hpp"


void
ebbrt::lrt::mem::init(unsigned num_cores)
{
  /* no init needed on ulnx */
}

void*
ebbrt::lrt::mem::malloc(size_t size, event::Location loc)
{
  /* location ignored since ulnx is single-threaded */
  return std::malloc(size);
}

void*
ebbrt::lrt::mem::memalign(size_t boundary, size_t size, event::Location loc)
{
  /* location ignored since ulnx is single-threaded */
  return std::malloc(size);
}
