#include <cstdio>
#include <cstdint>

#include "src/lib/ebblib/ebblib.hpp"
#include "src/lrt/boot.hpp"


void
ebblib::init()
{
  std::printf("ebblib init\n");

  ebbrt::lrt::boot::init();

  return;
}


