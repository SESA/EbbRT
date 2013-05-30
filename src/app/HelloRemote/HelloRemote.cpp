#include "app/HelloRemote/HelloRemote.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/Console/Console.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "ebb/PCI/PCI.hpp"
#ifdef __linux__
#include "lib/ebblib/ebblib.hpp"
#include "ebb/Ethernet/RawSocket.hpp"
#elif __ebbrt__
#include "ebb/Ethernet/VirtioNet.hpp"
#endif

namespace {
  ebbrt::EbbRoot* construct_root()
  {
    return new ebbrt::SharedRoot<ebbrt::HelloRemoteApp>;
  }
}

ebbrt::app::Config::InitEbb init_ebbs[] =
{
  {.id = ebbrt::memory_allocator,
   .create_root = ebbrt::SimpleMemoryAllocatorConstructRoot},
  {.id = ebbrt::ebb_manager,
   .create_root = ebbrt::PrimitiveEbbManagerConstructRoot},
  {.id = ebbrt::event_manager,
   .create_root = ebbrt::SimpleEventManager::ConstructRoot},
  {.id = ebbrt::app_ebb,
   .create_root = construct_root},
  {.id = ebbrt::console,
   .create_root = ebbrt::Console::ConstructRoot},
  {.id = ebbrt::message_manager,
   .create_root = ebbrt::MessageManager::ConstructRoot}
};

ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "MemoryAllocator", .id = 1},
  {.name = "EbbManager", .id = 2},
  {.name = "EventManager", .id = 3},
  {.name = "App", .id = 4},
  {.name = "Console", .id = 5},
  {.name = "MessageManager", .id = 6}
};

const ebbrt::app::Config ebbrt::app::config = {
  .node_id = 1,
  .num_init = sizeof(init_ebbs) / sizeof(Config::InitEbb),
  .init_ebbs = init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};

void
ebbrt::HelloRemoteApp::Start()
{
#ifdef __ebbrt__
  if (get_location() == 0) {
    pci = EbbRef<PCI>(ebb_manager->AllocateId());
    ebb_manager->Bind(PCI::ConstructRoot, pci);
    ethernet = EbbRef<Ethernet>(ebb_manager->AllocateId());
    ebb_manager->Bind(VirtioNet::ConstructRoot, ethernet);

    console->Write("Hello World\n");
  }
#elif __linux__
  ethernet = EbbRef<Ethernet>(ebb_manager->AllocateId());
  ebb_manager->Bind(RawSocket::ConstructRoot, ethernet);
  message_manager->StartListening();
  console->Write("Hello World\n");
#else
#error "Unsupported platform"
#endif
}

#ifdef __linux__
int main()
{
  ebblib::init();
  return 0;
}
#endif
