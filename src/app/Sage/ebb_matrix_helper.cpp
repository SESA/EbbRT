#include <memory>

#include "app/app.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

#ifdef __ebbrt__
#include "ebb/EbbManager/EbbManager.hpp"
#include "ebb/Ethernet/VirtioNet.hpp"
#include "ebb/PCI/PCI.hpp"
#include "app/Sage/Matrix.hpp"
#endif

#ifdef __linux__
#include "ebbrt.hpp"
#include "app/Sage/ebb_matrix_helper.hpp"
#endif

constexpr ebbrt::app::Config::InitEbb late_init_ebbs[] =
{
#ifdef __linux__
  { .name = "EbbManager" },
#endif
  { .name = "EventManager" },
  { .name = "MessageManager" },
  { .name = "Network" },
#ifdef __ebbrt__
  { .name = "Timer" },
  { .name = "Matrix" }
#endif
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
  {.name = "EventManager", .id = 5},
  {.name = "MessageManager", .id = 7},
  {.name = "Network", .id = 9},
  {.name = "Timer", .id = 10},

  //FIXME: get from config
  {.name = "Matrix", .id = 20}
};

const ebbrt::app::Config ebbrt::app::config = {
#ifdef __linux__
  .space_id = 0,
#else
  .space_id = 1,
#endif
#ifdef __ebbrt__
  .num_early_init = sizeof(early_init_ebbs) / sizeof(Config::InitEbb),
  .early_init_ebbs = early_init_ebbs,
#endif
  .num_late_init = sizeof(late_init_ebbs) / sizeof(Config::InitEbb),
  .late_init_ebbs = late_init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};

#ifdef __linux__
std::unique_ptr<ebbrt::EbbRT> instance;
std::unique_ptr<ebbrt::Context> context;
bool initialized = false;

void activate_context() {
  if (!initialized) {
    instance.reset(new ebbrt::EbbRT());
    context.reset(new ebbrt::Context(*instance));
    context->Activate();
    ebbrt::message_manager->StartListening();
    initialized = true;
  } else {
    context->Activate();
  }
}

void deactivate_context() {
  context->Deactivate();
}

void wait_for_future(ebbrt::Future<void>* fut) {
  fut->Then(ebbrt::launch::async, [](ebbrt::Future<void> complete){
      complete.Get();
      context->Break();
    });
  context->Loop(-1);
}

#else


void ebbrt::app::start() {
  pci = EbbRef<PCI>(ebb_manager->AllocateId());
  ebb_manager->Bind(PCI::ConstructRoot, pci);

  ethernet = EbbRef<Ethernet>(ebb_manager->AllocateId());
  ebb_manager->Bind(VirtioNet::ConstructRoot, ethernet);

  message_manager->StartListening();

  //FIXME: get from config
  auto matrix = EbbRef<Matrix>(20);
  matrix->Connect();
}
#endif
