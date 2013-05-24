#include <cstdio>
#include <cstdint>

#include "ebb/SharedRoot.hpp"
//#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
//#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "lib/ebblib/ebblib.hpp"
#include "lrt/boot.hpp"
#include "lrt/console.hpp"

namespace {
  ebbrt::EbbRoot* construct_root()
  {
    return new ebbrt::SharedRoot<ebbrt::FrontEndApp>;
  }
}

ebbrt::app::Config::InitEbb init_ebbs[] =
{
/*
  {.id = ebbrt::memory_allocator,
   .create_root = ebbrt::SimpleMemoryAllocatorConstructRoot},
  {.id = ebbrt::ebb_allocator,
   .create_root = ebbrt::PrimitiveEbbAllocatorConstructRoot},
  {.id = ebbrt::app_ebb,
   .create_root = construct_root}
   */
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


/*FIXME: this is ugly...*/
void
ebbrt::FrontEndApp::Start()
{
  //lock_.Lock();
  lrt::console::write("Hello World\n");
  //lock_.Unlock();
}
