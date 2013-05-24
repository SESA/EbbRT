#include "app/HelloRemote/HelloRemote.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/Console/Console.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"

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
  {.id = ebbrt::app_ebb,
   .create_root = construct_root},
  {.id = ebbrt::console,
   .create_root = ebbrt::Console::ConstructRoot}
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
    console->Write("Hello World\n");
    // pci = EbbRef<PCI>(ebb_manager->AllocateId());
    // ebb_manager->Bind(PCI::ConstructRoot, pci);
    // EbbRef<Ethernet> ethernet{ebb_manager->AllocateId()};
    // ebb_manager->Bind(VirtioNet::ConstructRoot, ethernet);
    // char* buffer = static_cast<char*>(ethernet->Allocate(64));

    // // destination mac address, broadcast
    // buffer[0] = 0xff;
    // buffer[1] = 0xff;
    // buffer[2] = 0xff;
    // buffer[3] = 0xff;
    // buffer[4] = 0xff;
    // buffer[5] = 0xff;

    // // ethertype (protocol number) that is free
    // buffer[12] = 0x88;
    // buffer[13] = 0x12;
    // std::strcpy(&buffer[14], "Hello World!\n");
    // ethernet->Send(buffer, 64);
  }
}
