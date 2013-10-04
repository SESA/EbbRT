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

#include <mutex>
#include <vector>

#include "app/app.hpp"
#include "ebb/Console/LocalConsole.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"

#ifdef __linux__
#include <iostream>
#include <thread>
#include "ebbrt.hpp"
#elif __ebbrt__
#include "ebb/Gthread/Gthread.hpp"
#include "ebb/Syscall/Syscall.hpp"
#include "lrt/bare/console.hpp"
#include "sync/spinlock.hpp"
#endif


#ifdef __ebbrt__
void
ebbrt::app::start()
{
  static Spinlock lock;
  lock.Lock();
  char str[] = "Hello Ebb\n";
  auto buf = ebbrt::console->Alloc(sizeof(str));
  std::memcpy(buf.data(), str, sizeof(str));
  ebbrt::console->Write(std::move(buf));
  lock.Unlock();
}
#endif


#ifdef __linux__

#include <stdlib.h>
#include <fstream>
#include "lib/fdt/libfdt.h"


/** this is included as a ugly kludge around our current configuration model. 
 * i.e., we need an id to statically construct ebbs before an fdt can be
 * parsed */
constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "EbbManager", .id = 2},
  {.name = "Console", .id = 5},
  {.name = "EventManager", .id = 6},
};
const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 0,
  .num_late_init = 0,
  .late_init_ebbs = 0,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};


int 
main(int argc, char* argv[] )
{
  char *fdt;
  if(argc < 2)
  {
      std::cout << "Usage: ./HelloEbb HelloEbb.dtb\n";
      std::exit(1);
  }
  std::ifstream is (argv[1], std::ifstream::binary);
  if (is) {
    // get length of file:
    is.seekg (0, is.end);
    int length = is.tellg();
    is.seekg (0, is.beg);

    fdt = new char [length+length];

    std::cout << "file opened, reading " << length << " characters... ";
    // read data as a block:
    is.read (fdt,length);

    if (is)
      std::cout << "all characters read successfully.\n";
    else
      std::cout << "error: only " << is.gcount() << " could be read\n";
    is.close();
  }else
  {
      std::cout << "No configuration found. Exiting\n";
      std::exit(1);
  }

  // ...fdt buffer contains the entire file...
  ebbrt::EbbRT instance((void *)fdt);
  ebbrt::Context context{instance};
  context.Activate();
  char str[] = "Hello Ebb\n";
  auto buf = ebbrt::console->Alloc(sizeof(str));
  std::memcpy(buf.data(), str, sizeof(str));
  ebbrt::console->Write(std::move(buf));
  context.Deactivate();

  std::cout << "Sucessful run\n";
  return 0;
}
#endif
