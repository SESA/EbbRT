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

#include "ebb/EbbManager/EbbManager.hpp"
#include "ebb/Ethernet/RawSocket.hpp"
#include "ebb/Network/LWIPNetwork.hpp"

int main(int argc, char** argv)
{
  ebbrt::EbbRT instance;

  ebbrt::Context context{instance};
  context.Activate();

  ebbrt::ethernet =
    ebbrt::EbbRef<ebbrt::Ethernet>(ebbrt::ebb_manager->AllocateId());
  ebbrt::ebb_manager->Bind(ebbrt::RawSocket::ConstructRoot, ebbrt::ethernet);

  auto network =
    ebbrt::EbbRef<ebbrt::Network>{ebbrt::ebb_manager->AllocateId()};
  ebbrt::ebb_manager->Bind(ebbrt::LWIPNetwork::ConstructRoot, network);

  network->Do();

  std::cout << "Ready" << std::endl;

  context.Loop(-1);

  return 0;
}
