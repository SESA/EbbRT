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

uint32_t
fdt_getint32(void* fdt, int root, const char *prop)
{
  int len;
  const unsigned char *id = (const unsigned char *)fdt_getprop(fdt, root, prop, &len);
  assert(len == 4);
  // machine independant byte ordering of litte-endian value
  return ((id[3]<<0)|(id[2]<<8)|(id[1]<<16)|(id[0]<<24));
}

int 
main(int argc, char* argv[] )
{
  if(argc < 2)
  {   std::cout << "Usage: <fdt binary> <freepool> <path/to/bare>\n";
      std::exit(1);
  }

  int n;
  FILE *fp;
  char out[1024];
  char *fdt = ebbrt::app::LoadFile(argv[1], &n);
  std::string config_path  = argv[1];
  std::string pool = argv[2];
  std::string bin_path = argv[3];
  std::string infocmd = "khinfo " + pool;

  /* **************************************************** */
//
//  char *ptr = ebbrt::app::LoadFile(argv[1], &n);
//
//  // Lets try updating an fdt, saving it to disk and verifying it.
//  if( fdt_check_header(ptr))
//    std::cout << "Bad header\n";
//
//  std::cout << "FTD initial size: " << fdt_totalsize(ptr) << std::endl;
//
//  uint32_t spaceid = fdt_getint32(ptr, 0, "space_id");
//
//  std::cout << "Space id: " << spaceid << std::endl;
//
//  fdt_setprop_u32(ptr, 0, "space_id", (spaceid+1));
//  fdt_setprop_string(ptr,0, "frontend_ip", "123.456.789");
//
//  std::cout << "Space id: " << fdt_getint32(ptr, 0, "space_id") << std::endl;
//   std::ofstream outfile ("new.txt",std::ofstream::binary);
//  // outfile.write (ptr,fdt_totalsize(ptr));
//   outfile.close();
//
  /* **************************************************** */

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
  std::string tmppath = "/home/jmcadden/tmp/";
  // update configuration
  const char *outptr = (const char *)ebbrt::config_handle->GetConfig();
  std::string newstr = tmppath + std::to_string(spaceid);
  std::ofstream outfile(newstr.c_str(),std::ofstream::binary);
  outfile.write (outptr,fdt_totalsize(outptr));
  outfile.close();

  fp = popen(infocmd.c_str(), "r"); 
  while (fgets(out, 1024, fp) != NULL)
  {
    std::string ip = out; 
    // strip newline char
    if (!ip.empty() && ip[ip.length()-1] == '\n') {
      ip.erase(ip.length()-1);
    }


    // allocate the node
    ebbrt::node_allocator->Allocate(ip, bin_path, newstr);
  }
  pclose(fp);
#endif

  context.Loop(-1);
  return 0;
}
