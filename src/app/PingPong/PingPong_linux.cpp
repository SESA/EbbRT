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

#include <iostream>

#include "app/app.hpp"
#include "ebb/EbbManager/EbbManager.hpp"
#ifndef UDP
#include "ebb/Ethernet/RawSocket.hpp"
#endif
#include "ebb/NodeAllocator/NodeAllocator.hpp"
#include "ebb/NodeAllocator/Kittyhawk.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "ebbrt.hpp"

/****************************/
// Static ebb ulnx kludge 
/****************************/
constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "EbbManager", .id = 2},
  {.name = "EventManager", .id = 5},
  {.name = "Console", .id = 6},
  {.name = "MessageManager", .id = 7},
  {.name = "Echo", .id = 8}
};

const ebbrt::app::Config ebbrt::app::config = {
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};
/****************************/

int 
main(int argc, char* argv[] )
{
  if(argc < 2)
  {
      std::cout << "Usage: Pass fdt binary in as first argument\n";
      std::exit(1);
  }

  int n;
  char *fdt = ebbrt::app::LoadConfig(argv[1], &n);

  ebbrt::EbbRT instance(fdt);

  ebbrt::Context context{instance};
  context.Activate();

#ifndef UDP
  ebbrt::ethernet =
    ebbrt::EbbRef<ebbrt::Ethernet>(ebbrt::ebb_manager->AllocateId());
  ebbrt::ebb_manager->Bind(ebbrt::RawSocket::ConstructRoot, ebbrt::ethernet);
#endif
  ebbrt::node_allocator = 
    ebbrt::EbbRef<ebbrt::NodeAllocator>(ebbrt::ebb_manager->AllocateId());
  ebbrt::ebb_manager->Bind(ebbrt::Kittyhawk::ConstructRoot, ebbrt::node_allocator);

  ebbrt::message_manager->StartListening();

  ebbrt::node_allocator->Allocate();

  std::cout << "Ready" << std::endl;

  context.Loop(-1);

  return 0;
}
