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
#ifndef EBBRT_EBB_ETHERNET_ETHERNET_HPP
#define EBBRT_EBB_ETHERNET_ETHERNET_HPP

#include <functional>

#include "ebb/ebb.hpp"
#include "misc/buffer.hpp"

namespace ebbrt {
  class Ethernet : public EbbRep {
  public:
    class Header {
    public:
      uint8_t destination[6];
      uint8_t source[6];
      uint16_t ethertype;
    } __attribute__((packed));
    virtual void Send(BufferList buffers,
                      std::function<void()> cb = nullptr) = 0;
    virtual const uint8_t* MacAddress() = 0;
    virtual void SendComplete() = 0;
    virtual void Register(uint16_t ethertype,
                          std::function<void(const char*, size_t)> func) = 0;
    virtual void Receive() = 0;
  protected:
    Ethernet(EbbId id) : EbbRep{id} {}
  };
  extern EbbRef<Ethernet> ethernet;
}

#endif
