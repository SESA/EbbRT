/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "app/app.hpp"
#include "ebb/Console/RemoteConsole.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#include "ebb/Gthread/Gthread.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "ebb/Syscall/Syscall.hpp"
#ifdef __linux__
#include "ebbrt.hpp"
#include "ebb/Ethernet/RawSocket.hpp"
#elif __ebbrt__
#include "ebb/PCI/PCI.hpp"
#include "ebb/Ethernet/VirtioNet.hpp"
#endif

constexpr ebbrt::app::Config::InitEbb init_ebbs[] =
{
  {
    .create_root = ebbrt::SimpleMemoryAllocatorConstructRoot,
    .name = "MemoryAllocator"
  },
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
    .create_root = ebbrt::SimpleEventManager::ConstructRoot,
    .name = "EventManager"
  },
  {
    .create_root = ebbrt::RemoteConsole::ConstructRoot,
    .name = "Console"
  },
  {
    .create_root = ebbrt::MessageManager::ConstructRoot,
    .name = "MessageManager"
  }
};

constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "MemoryAllocator", .id = 1},
  {.name = "EbbManager", .id = 2},
  {.name = "Gthread", .id = 3},
  {.name = "Syscall", .id = 4},
  {.name = "EventManager", .id = 5},
  {.name = "Console", .id = 6},
  {.name = "MessageManager", .id = 7}
};

const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 1,
#ifdef __ebbrt__
  .num_early_init = 4,
#endif
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
  ebbrt::EbbRT instance;

  ebbrt::Context context{instance};
  context.Loop(-1);

  return 0;
}
#endif
