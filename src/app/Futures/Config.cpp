#include "app/app.hpp"
#include "ebb/Console/LocalConsole.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#include "ebb/Gthread/Gthread.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "ebb/Syscall/Syscall.hpp"

#ifdef __linux__
#include "ebbrt.hpp"
#endif

constexpr ebbrt::app::Config::InitEbb init_ebbs[] =
{
#ifdef __ebbrt__
  {
    .create_root = ebbrt::SimpleMemoryAllocatorConstructRoot,
    .name = "MemoryAllocator"
  },
#endif
  {
    .create_root = ebbrt::PrimitiveEbbManagerConstructRoot,
    .name = "EbbManager"
  },
#ifdef __ebbrt__
  {
    .create_root = ebbrt::Gthread::ConstructRoot,
    .name = "Gthread"
  },
  {
    .create_root = ebbrt::Syscall::ConstructRoot,
    .name = "Syscall"
  },
#endif
  {
    .create_root = ebbrt::LocalConsole::ConstructRoot,
    .name = "Console"
  },
  {
    .create_root = ebbrt::SimpleEventManager::ConstructRoot,
    .name = "EventManager"
  }
};

constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "MemoryAllocator", .id = 1},
  {.name = "EbbManager", .id = 2},
  {.name = "Gthread", .id = 3},
  {.name = "Syscall", .id = 4},
  {.name = "Console", .id = 5},
  {.name = "EventManager", .id = 6}
};

const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 0,
#ifdef __ebbrt__
  .num_early_init = 4,
#endif
  .num_init = sizeof(init_ebbs) / sizeof(Config::InitEbb),
  .init_ebbs = init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};
