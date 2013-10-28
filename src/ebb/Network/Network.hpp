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
#ifndef EBBRT_EBB_NETWORK_NETWORK_HPP
#define EBBRT_EBB_NETWORK_NETWORK_HPP

#include "ebb/ebb.hpp"
#include "lrt/config.hpp"

namespace ebbrt {
class Network : public EbbRep {
public:
  virtual void InitPing() = 0;
  virtual void InitEcho() = 0;
  virtual void RegisterUDP(uint16_t port,
                           std::function<void(Buffer, NetworkId)> cb) = 0;
  virtual void SendUDP(Buffer buffer, NetworkId to, uint16_t port) = 0;

protected:
  Network(EbbId id) : EbbRep{ id } {}
};
const EbbRef<Network> network =
    EbbRef<Network>(lrt::config::find_static_ebb_id(nullptr,"Network"));
}

#endif
