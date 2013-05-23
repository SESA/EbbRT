#include <cstdio>
#include <cstdint>

#include "ebb/SharedRoot.hpp"
#include "ebb/EbbAllocator/PrimitiveEbbAllocator.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "src/lib/ebblib/ebblib.hpp"
#include "src/lrt/boot.hpp"

namespace {
  ebbrt::EbbRoot* construct_root()
  {
    return new ebbrt::SharedRoot<ebbrt::FrontEndApp>;
  }
}

ebbrt::app::Config::InitEbb init_ebbs[] =
{
  {.id = ebbrt::memory_allocator,
   .create_root = ebbrt::SimpleMemoryAllocatorConstructRoot},
  {.id = ebbrt::ebb_allocator,
   .create_root = ebbrt::PrimitiveEbbAllocatorConstructRoot},
  {.id = ebbrt::app_ebb,
   .create_root = construct_root}
};

const ebbrt::app::Config ebbrt::app::config = {
  .node_id = 0,
  .num_init = 3,
  .init_ebbs = init_ebbs
};


void
ebblib::init()
{
  std::printf("ebblib init\n");

  ebbrt::lrt::boot::init();

  return;
}


