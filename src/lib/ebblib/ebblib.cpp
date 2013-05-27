#include <cstdio>
#include <cstdint>

#include "lib/ebblib/ebblib.hpp"
#include "lrt/boot.hpp"
#include "lrt/console.hpp"


void
ebblib::init()
{
  std::printf("ebblib init\n");

  ebbrt::lrt::boot::init();

  return;
}

