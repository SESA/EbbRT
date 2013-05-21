#include <stdio.h>
#include <cstdint>

#include "lrt/mem.hpp"


void
ebbrt::lrt::mem::init()
{
  return;
}

void*
ebbrt::lrt::mem::malloc(size_t size, event::Location loc);
{
  /* kludge */
  return malloc(size);
}

void*
ebbrt::lrt::mem::memalign(size_t boundary, size_t size, event::Location loc);
{
  /* kludge */
  return memalign(boundary, size);
}
