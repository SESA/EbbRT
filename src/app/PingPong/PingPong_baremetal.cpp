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
#include <string>
#include <cstdlib>

#include "app/app.hpp"
#include "ebb/Config/Config.hpp"
#include "ebb/Config/Fdt.hpp"
#include "ebb/EbbManager/EbbManager.hpp"
#include "ebb/Ethernet/VirtioNet.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "ebb/PCI/PCI.hpp"
#include "lrt/config.hpp"
#include "lrt/bare/boot.hpp"

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

  config_handle = EbbRef<Config>(ebb_manager->AllocateId());
  ebb_manager->Bind(Fdt::ConstructRoot, config_handle);
  config_handle->SetConfig(lrt::boot::fdt);

  size_t pos = 0;
  int count = 0;
  int ip[4];
  std::string s = config_handle->GetString("/", "frontend_ip");
  while ((pos = s.find(".")) != std::string::npos) {
    std::string t = s.substr(0, pos);
    ip[count++] = atoi(t.c_str());
    s.erase(0, pos + 1);
  } ip[count] = atol(s.c_str());

  id.addr = ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];

#else
  id.mac_addr[0] = 0xff;
  id.mac_addr[1] = 0xff;
  id.mac_addr[2] = 0xff;
  id.mac_addr[3] = 0xff;
  id.mac_addr[4] = 0xff;
  id.mac_addr[5] = 0xff;
#endif
  //TODO: configuration needs to be resolved here
  message_manager->Send(id, lrt::config::find_static_ebb_id(nullptr,"Echo"),
                        std::move(buf));
}
