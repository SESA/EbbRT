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

#include <vector>

#include "app/app.hpp"
#include "app/HiddenRegister/Hidden.hpp"
#include "ebb/EbbManager/EbbManager.hpp"
#include "ebb/Console/Console.hpp"

#ifdef __linux__
#include <iostream>
#include "ebbrt.hpp"
#endif

constexpr ebbrt::app::Config::InitEbb late_init_ebbs[] =
{
#ifdef __linux__
  { .name = "EbbManager" },
#endif
  { .name = "Console" },
  { .name = "EventManager" }
};

#ifdef __ebbrt__
constexpr ebbrt::app::Config::InitEbb early_init_ebbs[] =
{
  { .name = "MemoryAllocator" },
  { .name = "EbbManager" },
  { .name = "Gthread" },
  { .name = "Syscall" }
};
#endif

constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "MemoryAllocator", .id = 1},
  {.name = "EbbManager", .id = 2},
  {.name = "Gthread", .id = 3},
  {.name = "Syscall", .id = 4},
  {.name = "Console", .id = 5},
  {.name = "EventManager", .id = 6}
};

const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 1,
#ifdef __ebbrt__
  .num_early_init = sizeof(early_init_ebbs) / sizeof(Config::InitEbb),
  .early_init_ebbs = early_init_ebbs,
#endif
  .num_late_init = sizeof(late_init_ebbs) / sizeof(Config::InitEbb),
  .late_init_ebbs = late_init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};

void
test()
{
  ebbrt::EbbRef<ebbrt::Hidden> hidden {ebbrt::ebb_manager->AllocateId()};
  ebbrt::ebb_manager->Bind(ebbrt::Hidden::ConstructRoot, hidden);

  hidden->NoReturn();

  ebbrt::EbbRef<ebbrt::Hidden> hidden2 {ebbrt::ebb_manager->AllocateId()};
  ebbrt::ebb_manager->Bind(ebbrt::Hidden::ConstructRoot, hidden2);

  hidden2->SmallPODReturn();

  ebbrt::EbbRef<ebbrt::Hidden> hidden3 {ebbrt::ebb_manager->AllocateId()};
  ebbrt::ebb_manager->Bind(ebbrt::Hidden::ConstructRoot, hidden3);

  hidden3->BigPODReturn();
}

#ifdef __ebbrt__
bool ebbrt::app::multi = true;

void
ebbrt::app::start()
{
  test();
}
#endif

#ifdef __linux__
int main()
{
  ebbrt::EbbRT instance;

  ebbrt::Context context{instance};
  context.Activate();

  test();

  context.Deactivate();

  return 0;
}
#endif
