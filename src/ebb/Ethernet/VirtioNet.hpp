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

#include <unordered_map>

#include "ebb/Ethernet/Ethernet.hpp"
#include "device/virtio.hpp"
#include "sync/spinlock.hpp"

namespace ebbrt {
  class VirtioNet : public Ethernet {
  public:
    static EbbRoot* ConstructRoot();
    VirtioNet(EbbId id);
    Buffer Alloc(size_t size) override;
    void Send(Buffer buffer) override;
    const char* MacAddress() override;
    void SendComplete() override;
    void Register(uint16_t ethertype,
                  std::function<void(Buffer)> func) override;
    void Receive() override;
  private:
    struct Ring {
      uint16_t size;
      uint16_t free_head;
      uint16_t num_free;
      uint16_t last_used;
      virtio::QueueDescriptor* descs;
      virtio::Available* available;
      virtio::Used* used;
    };

    void InitRing(Ring& ring);

    uint16_t io_addr_;
    Ring receive_;
    Ring send_;
    bool msix_enabled_;
    char mac_address_[6];
    char empty_header_[10];
    std::unordered_map<uint16_t, Buffer> buffer_map_;
    std::unordered_map<uint16_t,
                       std::function<void(Buffer)> > rcv_map_;
  };
}

#endif
