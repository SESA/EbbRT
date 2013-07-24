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

constexpr ebbrt::app::Config::InitEbb late_init_ebbs[] =
{
#ifdef __linux__
  { .name = "EbbManager" },
#endif
  { .name = "Console" },
  { .name = "EventManager" }
};

#ifdef __ebbrt__
constexpr ebbrt::app::Config::InitEbb early_init_ebbs[] =
{
  { .name = "MemoryAllocator" },
  { .name = "EbbManager" },
  { .name = "Gthread" },
  { .name = "Syscall" }
};
#endif

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
  .num_early_init = sizeof(early_init_ebbs) / sizeof(Config::InitEbb),
  .early_init_ebbs = early_init_ebbs,
#endif
  .num_late_init = sizeof(late_init_ebbs) / sizeof(Config::InitEbb),
  .late_init_ebbs = late_init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};
