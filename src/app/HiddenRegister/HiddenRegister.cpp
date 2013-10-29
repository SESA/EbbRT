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
void
ebbrt::app::start()
{
  test();
}
#endif

#ifdef __linux__
/****************************/
// Static ebb ulnx kludge
/****************************/
constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "EbbManager", .id = 2},
  {.name = "Console", .id = 5},
  {.name = "EventManager", .id = 6},
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
      std::cout << "Usage: dtb as first argument \n";
      std::exit(1);
  }

  int n;
  char *fdt = ebbrt::app::LoadFile(argv[1], &n);

  // ...fdt buffer contains the entire file...
  ebbrt::EbbRT instance((void *)fdt);

  ebbrt::Context context{instance};
  context.Activate();

  test();

  context.Deactivate();

  return 0;
}
#endif
