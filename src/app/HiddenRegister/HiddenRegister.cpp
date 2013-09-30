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
