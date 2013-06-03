#include "app/app.hpp"
#include "ebb/Console/Console.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#ifdef __linux__
#include "lib/ebblib/ebblib.hpp"
#include "ebb/Ethernet/RawSocket.hpp"
#elif __ebbrt__
#include "ebb/PCI/PCI.hpp"
#include "ebb/Ethernet/VirtioNet.hpp"
#endif

const ebbrt::app::Config::InitEbb init_ebbs[] =
{
  {
    .create_root = ebbrt::SimpleMemoryAllocatorConstructRoot,
    .id = ebbrt::memory_allocator
  },
  {
    .create_root = ebbrt::PrimitiveEbbManagerConstructRoot,
    .id = ebbrt::ebb_manager
  },
  {
    .create_root = ebbrt::SimpleEventManager::ConstructRoot,
    .id = ebbrt::event_manager
  },
  {
    .create_root = ebbrt::Console::ConstructRoot,
    .id = ebbrt::console
  },
  {
    .create_root = ebbrt::MessageManager::ConstructRoot,
    .id = ebbrt::message_manager
  }
};

const ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "MemoryAllocator", .id = 1},
  {.name = "EbbManager", .id = 2},
  {.name = "EventManager", .id = 3},
  {.name = "Console", .id = 4},
  {.name = "MessageManager", .id = 5}
};

const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 1,
  .num_init = sizeof(init_ebbs) / sizeof(Config::InitEbb),
  .init_ebbs = init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};

void
ebbrt::app::start()
{
#ifdef __ebbrt__
  pci = EbbRef<PCI>(ebb_manager->AllocateId());
  ebb_manager->Bind(PCI::ConstructRoot, pci);
  ethernet = EbbRef<Ethernet>(ebb_manager->AllocateId());
  ebb_manager->Bind(VirtioNet::ConstructRoot, ethernet);
#elif __linux__
  ethernet = EbbRef<Ethernet>(ebb_manager->AllocateId());
  ebb_manager->Bind(RawSocket::ConstructRoot, ethernet);
  message_manager->StartListening();
#endif
  console->Write("Hello World\n");
}

#ifdef __linux__
int main()
{
  ebblib::init();
  return 0;
}
#endif
