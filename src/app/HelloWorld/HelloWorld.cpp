#include "app/HelloWorld/HelloWorld.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "lrt/console.hpp"

namespace {
  ebbrt::EbbRoot* construct_root()
  {
    return new ebbrt::SharedRoot<ebbrt::HelloWorldApp>;
  }
}

ebbrt::app::Config::InitEbb init_ebbs[] =
{
  {.id = ebbrt::memory_allocator,
   .create_root = ebbrt::SimpleMemoryAllocatorConstructRoot},
  {.id = ebbrt::ebb_manager,
   .create_root = ebbrt::PrimitiveEbbManagerConstructRoot},
  {.id = ebbrt::app_ebb,
   .create_root = construct_root}
};

const ebbrt::app::Config ebbrt::app::config = {
  .node_id = 0,
  .num_init = 3,
  .init_ebbs = init_ebbs
};

#include "lrt/assert.hpp"

void
ebbrt::HelloWorldApp::Start()
{
  lock_.Lock();
  lrt::console::write("Hello World\n");
  lock_.Unlock();
}
