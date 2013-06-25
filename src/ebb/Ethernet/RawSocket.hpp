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

    RawSocket();
    void Send(BufferList buffers,
              std::function<void()> cb = nullptr) override;
    const uint8_t* MacAddress() override;
    void SendComplete() override;
    void Register(uint16_t ethertype,
                  std::function<void(const char*, size_t)> func) override;
    void Receive() override;
  private:
    uint8_t mac_addr_[6];
    pcap_t* pdev_;
    std::unordered_map<uint16_t,
                       std::function<void(const char*, size_t)> > map_;
  };
}

#endif
