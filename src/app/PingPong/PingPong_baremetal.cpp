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
#include "ebb/EbbManager/EbbManager.hpp"
#include "ebb/Ethernet/VirtioNet.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "ebb/PCI/PCI.hpp"
#include "lrt/config.hpp"

void
ebbrt::app::start()
{
  pci = EbbRef<PCI>(ebb_manager->AllocateId());
  ebb_manager->Bind(PCI::ConstructRoot, pci);

  ethernet = EbbRef<Ethernet>(ebb_manager->AllocateId());
  ebb_manager->Bind(VirtioNet::ConstructRoot, ethernet);

  message_manager->StartListening();

  const char cbuf[] = "PING\n";
  auto buf = message_manager->Alloc(sizeof(cbuf));
  std::memcpy(buf.data(), cbuf, sizeof(cbuf));
  NetworkId id;
#ifdef UDP
  id.addr =
    192 << 24 | 168 << 16 | 0 << 8 | 3;
#else
  id.mac_addr[0] = 0xff;
  id.mac_addr[1] = 0xff;
  id.mac_addr[2] = 0xff;
  id.mac_addr[3] = 0xff;
  id.mac_addr[4] = 0xff;
  id.mac_addr[5] = 0xff;
  //TODO: configuration needs to be resolved here
  message_manager->Send(id, lrt::trans::find_static_ebb_id(nullptr,"Echo"),
                        std::move(buf));
}
