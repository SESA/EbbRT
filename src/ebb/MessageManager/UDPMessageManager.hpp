/*
  Ebb RT: Distributed, Elastic, Runtime
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
#ifndef EBBRT_EBB_MESSAGEMANAGER_UDPMESSAGEMANAGER_HPP
#define EBBRT_EBB_MESSAGEMANAGER_UDPMESSAGEMANAGER_HPP

#include "ebb/MessageManager/MessageManager.hpp"
#include "misc/buffer.hpp"

namespace ebbrt {
  class UDPMessageManager : public MessageManager {
    uint16_t port_;
  public:
    static EbbRoot* ConstructRoot();
    UDPMessageManager(EbbId id);
    virtual Buffer Alloc(size_t size) override;
    virtual void Send(NetworkId to,
                      EbbId ebb,
                      Buffer buffer) override;
    virtual void StartListening() override;
  };
}
#endif
