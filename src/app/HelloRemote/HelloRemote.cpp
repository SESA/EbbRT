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
#include "ebb/Syscall/Syscall.hpp"

#ifndef __bg__
#include "ebb/MessageManager/EthernetMessageManager.hpp"
#else
#include "ebb/MessageManager/MPIMessageManager.hpp"
#endif

#ifdef __linux__
#include "ebbrt.hpp"
#ifndef __bg__
#include "ebb/Ethernet/RawSocket.hpp"
#else
#include <iostream>
#include <mpi.h>
#endif
#elif __ebbrt__
#include "ebb/PCI/PCI.hpp"
#include "ebb/Ethernet/VirtioNet.hpp"
#endif

/****************************/
// Static ebb ulnx kludge 
/****************************/
constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "EbbManager", .id = 2},
  {.name = "EventManager", .id = 5},
  {.name = "Console", .id = 6},
  {.name = "MessageManager", .id = 7}
};
const ebbrt::app::Config ebbrt::app::config = {
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};
/****************************/

#include <cstdio>

#ifdef __ebbrt__
void
ebbrt::app::start()
{
  pci = EbbRef<PCI>(ebb_manager->AllocateId());
  ebb_manager->Bind(PCI::ConstructRoot, pci);
  ethernet = EbbRef<Ethernet>(ebb_manager->AllocateId());
  ebb_manager->Bind(VirtioNet::ConstructRoot, ethernet);
  const char str[] = "Hello World (remote)\n";
  auto buf = ebbrt::console->Alloc(sizeof(str));
  std::memcpy(buf.data(), str, sizeof(str));
  ebbrt::console->Write(std::move(buf));
}
#endif

#ifdef __linux__
int
main(int argc, char* argv[] )
{
  if(argc < 2)
  {
    std::cout << "Usage: fdb as first argument \n";
    std::exit(1);
  }
  int n;
  char *fdt = ebbrt::app::LoadConfig(argv[1], &n);

#ifdef __bg__
  if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
    std::cerr << "MPI_Init failed" << std::endl;
    return -1;
  }
#endif
  ebbrt::EbbRT instance((void *)fdt);

  ebbrt::Context context{instance};
  context.Activate();
#ifndef __bg__
  ebbrt::ethernet = ebbrt::EbbRef<ebbrt::Ethernet>(ebbrt::ebb_manager->AllocateId());
  ebbrt::ebb_manager->Bind(ebbrt::RawSocket::ConstructRoot, ebbrt::ethernet);
  ebbrt::message_manager->StartListening();
  const char str[] = "Hello World (frontend)\n";
  auto buf = ebbrt::console->Alloc(sizeof(str));
  std::memcpy(buf.data(), str, sizeof(str));
  ebbrt::console->Write(std::move(buf));
#else
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    ebbrt::message_manager->StartListening();
  }
  if (MPI_Barrier(MPI_COMM_WORLD) != MPI_SUCCESS) {
    std::cerr << "MPI_Barrier failed" << std::endl;
    return -1;
  }
  if (rank == 0) {
    const char str[] = "Hello World (frontend)\n";
    auto buf = ebbrt::console->Alloc(sizeof(str));
    std::memcpy(buf.data(), str, sizeof(str));
    ebbrt::console->Write(std::move(buf));
  } else {
    const char str[] = "Hello World (remote)\n";
    auto buf = ebbrt::console->Alloc(sizeof(str));
    std::memcpy(buf.data(), str, sizeof(str));
    ebbrt::console->Write(std::move(buf));
  }
#endif
  context.Loop(-1);

  return 0;
}
#endif
