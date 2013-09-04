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
#ifndef EBBRT_EBB_ETHERNET_RAWSOCKET_HPP
#define EBBRT_EBB_ETHERNET_RAWSOCKET_HPP

#include <unordered_map>

#include <pcap.h>

#include "ebb/Ethernet/Ethernet.hpp"

namespace ebbrt {
  class RawSocket : public Ethernet {
  public:
    static EbbRoot* ConstructRoot();

    RawSocket(EbbId id);
    Buffer Alloc(size_t size) override;
    void Send(Buffer buffer, const char* to,
              const char* from, uint16_t ethertype) override;
    const char* MacAddress() override;
    void SendComplete() override;
    void Register(uint16_t ethertype,
                  std::function<void(Buffer, const char[6])> func) override;
    void Receive() override;
  private:
    char mac_addr_[6];
    pcap_t* pdev_;
    std::unordered_map<uint16_t,
                       std::function<void(Buffer, const char[6])> > map_;
  };
}

#endif
