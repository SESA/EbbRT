#include <array>

#include "app/app.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "lrt/console.hpp"

#ifdef __linux__
#include "lib/ebblib/ebblib.hpp"
#endif

const ebbrt::app::Config::InitEbb init_ebbs[] =
{
  {
    .create_root = ebbrt::SimpleMemoryAllocatorConstructRoot,
    .id = ebbrt::memory_allocator
  },
  {
    .create_root = ebbrt::PrimitiveEbbManagerConstructRoot,
    .id = ebbrt::ebb_manager,
  }
};

const ebbrt::app::Config::StaticEbbId static_ebbs[] =
{
  {.name = "MemoryAllocator", .id = 1},
  {.name = "EbbManager", .id = 2},
};

const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 0,
  .num_init = sizeof(init_ebbs) / sizeof(Config::InitEbb),
  .init_ebbs = init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};

#ifdef __ebbrt__
bool ebbrt::app::multi = true;
#endif

void
ebbrt::app::start()
{
#ifdef __ebbrt__
  static Spinlock lock;
  lock.Lock();
#endif
  lrt::console::write("Hello World\n");
#ifdef __ebbrt__
  lock.Unlock();
#endif
}

#ifdef __linux__
int main()
{
  ebblib::init();
  return 0;
}
#endif
