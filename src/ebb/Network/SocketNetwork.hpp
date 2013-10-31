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
#ifndef EBBRT_EBB_NETWORK_SOCKETNETWORK_HPP
#define EBBRT_EBB_NETWORK_SOCKETNETWORK_HPP

#include "ebb/Network/Network.hpp"

namespace ebbrt {
class SocketNetwork : public Network {
public:
  static EbbRoot *ConstructRoot();

  SocketNetwork(EbbId id);

  virtual void InitPing() override;
  virtual void InitEcho() override;
  virtual uint16_t
  RegisterUDP(uint16_t port,
              std::function<void(Buffer, NetworkId)> cb) override;
  virtual void SendUDP(Buffer buffer, NetworkId to, uint16_t port) override;

private:
  int send_fd_;
};
}

#endif
