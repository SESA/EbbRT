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
` along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <mutex>
#include <vector>

#include "app/app.hpp"
#include "ebb/Console/LocalConsole.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"

#include "ebbrt.hpp"

constexpr ebbrt::app::Config::InitEbb late_init_ebbs[] =
{
  { .name = "EbbManager" },
  { .name = "Console" },
  { .name = "EventManager" }
};

constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "MemoryAllocator", .id = 1},
  {.name = "EbbManager", .id = 2},
  {.name = "Gthread", .id = 3},
  {.name = "Syscall", .id = 4},
  {.name = "Console", .id = 5},
  {.name = "EventManager", .id = 6}
};

const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 0,
  .num_late_init = sizeof(late_init_ebbs) / sizeof(Config::InitEbb),
  .late_init_ebbs = late_init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};

extern "C" void print_hello_ebb()
{
  ebbrt::EbbRT instance;
  ebbrt::Context context{instance};

  context.Activate();

  char str[] = "Hello Ebb\n";
  auto buf = ebbrt::console->Alloc(sizeof(str));
  std::memcpy(buf.data(), str, sizeof(str));
  ebbrt::console->Write(std::move(buf));

  context.Deactivate();
}
