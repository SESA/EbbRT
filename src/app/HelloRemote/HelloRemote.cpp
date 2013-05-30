#include "app/HelloRemote/HelloRemote.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/Console/Console.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/Ethernet/VirtioNet.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "ebb/PCI/PCI.hpp"

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

const ebbrt::app::Config ebbrt::app::config = {
  .node_id = 1,
  .num_init = sizeof(init_ebbs) / sizeof(Config::InitEbb),
  .init_ebbs = init_ebbs
};

void
ebbrt::HelloRemoteApp::Start()
{
  if (get_location() == 0) {
    pci = EbbRef<PCI>(ebb_manager->AllocateId());
    ebb_manager->Bind(PCI::ConstructRoot, pci);
    ethernet = EbbRef<Ethernet>(ebb_manager->AllocateId());
    ebb_manager->Bind(VirtioNet::ConstructRoot, ethernet);

    console->Write("Hello World\n");
  }
}
