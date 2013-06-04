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
#ifndef EBBRT_EBB_ETHERNET_VIRTIONET_HPP
#define EBBRT_EBB_ETHERNET_VIRTIONET_HPP

#include <map>

#include "ebb/Ethernet/Ethernet.hpp"
#include "device/virtio.hpp"
#include "sync/spinlock.hpp"

namespace ebbrt {
  class VirtioNet : public Ethernet {
  public:
    static EbbRoot* ConstructRoot();
    VirtioNet();
    void Send(BufferList buffers,
              std::function<void()> cb = nullptr) override;
    const uint8_t* MacAddress() override;
    void SendComplete() override;
    void Register(uint16_t ethertype,
                  std::function<void(const uint8_t*, size_t)> func) override;
    void Receive() override;
  private:
    uint16_t io_addr_;
    uint16_t next_free_;
    uint16_t next_available_;
    uint16_t last_sent_used_ = 0;
    Spinlock lock_;
    uint16_t send_max_;
    bool msix_enabled_;
    uint8_t mac_address_[6];
    virtio::QueueDescriptor* send_descs_;
    virtio::Available* send_available_;
    virtio::Used* send_used_;
    char empty_header_[10];
    std::map<uint16_t, std::function<void()> > cb_map_;
  };
}

#endif
