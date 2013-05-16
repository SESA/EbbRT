#include <sstream>

#include "app/HelloWorld/HelloWorld.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/EbbAllocator/PrimitiveEbbAllocator.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "lrt/console.hpp"
#include "misc/pci.hpp"

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

ebbrt::HelloWorldApp::HelloWorldApp() : lock_(ATOMIC_FLAG_INIT)
{
}

void
ebbrt::HelloWorldApp::Start()
{
  if (get_location() == 0) {
    std::ostringstream sstream;
    pci::enumerate_all_buses(sstream);
    const char* s = sstream.str().c_str();
    lrt::console::write(s);
  }
}
