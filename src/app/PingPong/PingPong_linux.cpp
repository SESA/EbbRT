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
#include <string>

#include "app/app.hpp"
#include "ebb/Config/Fdt.hpp"
#include "ebb/EbbManager/EbbManager.hpp"
#ifndef UDP
#include "ebb/Ethernet/RawSocket.hpp"
#endif
#include "ebb/NodeAllocator/Kittyhawk.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "ebbrt.hpp"
#include "lib/fdt/libfdt.h"

/****************************/
// Static ebb ulnx kludge 
/****************************/
constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "EbbManager", .id = 2},
  {.name = "EventManager", .id = 5},
  {.name = "Console", .id = 6},
  {.name = "MessageManager", .id = 7},
  {.name = "Echo", .id = 8},
  {.name = "Network", .id = 9},
  {.name = "Timer", .id = 10}
};

const ebbrt::app::Config ebbrt::app::config = {
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};
/****************************/

namespace {
     unsigned const node_count = 10;
};

int 
main(int argc, char* argv[] )
{
  if(argc < 2)
  {   std::cout << "Usage: <fdt binary> <path/to/bare>\n";
      std::exit(1);
  }

  int n;
  char *fdt = ebbrt::app::LoadFile(argv[1], &n);
  std::string config_path  = argv[1];
  std::string bin_path = argv[2];

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

  ebbrt::config_handle =
    ebbrt::EbbRef<ebbrt::Config>(ebbrt::ebb_manager->AllocateId());
  ebbrt::ebb_manager->Bind(ebbrt::Fdt::ConstructRoot, ebbrt::config_handle);


  ebbrt::message_manager->StartListening();


  char *ptr = ebbrt::app::LoadFile(argv[1], &n);
  ebbrt::config_handle->SetConfig(ptr);
  uint32_t spaceid = ebbrt::config_handle->GetInt32("/", "space_id");
  ebbrt::config_handle->SetString("/", "frontend_ip", "10.255.0.1");


#ifdef UDP
  std::string tmppath = "/scratch/"; // Ugh..
  // update configuration
  const char *outptr = (const char *)ebbrt::config_handle->GetConfig();
  std::string newconfig = tmppath + std::to_string(spaceid);
  std::ofstream outfile(newconfig.c_str(),std::ofstream::binary);
  outfile.write (outptr,fdt_totalsize(outptr));
  outfile.close();

  for(unsigned int i=0; i<node_count; i++)
    ebbrt::node_allocator->Allocate(bin_path, newconfig);
#endif

  context.Loop(-1);
  return 0;
}
