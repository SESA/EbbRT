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
#ifndef EBBRT_EBB_NETWORK_LWIPNETWORK_HPP
#define EBBRT_EBB_NETWORK_LWIPNETWORK_HPP

#include <queue>

#include "ebb/Network/Network.hpp"
#include "lwip/udp.h"

namespace ebbrt {
class LWIPNetwork : public Network {
public:
  static EbbRoot *ConstructRoot();

  LWIPNetwork(EbbId id);

  virtual void InitPing() override;
  virtual void InitEcho() override;
  virtual void RegisterUDP(uint16_t port,
                           std::function<void(Buffer, NetworkId)> cb) override;
  virtual void SendUDP(Buffer buffer, NetworkId to, uint16_t port) override;
  virtual void EmptyQueue();
private:
  virtual void RecvPacket(Buffer buffer);

  std::function<void()> tcp_timer_func_;
  std::function<void()> ip_timer_func_;
  std::function<void()> arp_timer_func_;
  std::function<void()> dhcp_coarse_timer_func_;
  std::function<void()> dhcp_fine_timer_func_;
  std::function<void()> dns_timer_func_;

  std::queue<std::tuple<Buffer, NetworkId, uint16_t> > queue_;
  udp_pcb send_pcb_;
};
}

#endif
