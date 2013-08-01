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
#include <algorithm>

#include "device/virtio.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/Ethernet/VirtioNet.hpp"
#include "ebb/EventManager/EventManager.hpp"
#include "ebb/MemoryAllocator/MemoryAllocator.hpp"
#include "ebb/PCI/PCI.hpp"
#include "lrt/bare/assert.hpp"
#include "sync/compiler.hpp"

namespace {
  union Features {
    uint32_t raw;
    struct {
      uint32_t csum :1;
      uint32_t guest_csum :1;
      uint32_t :3;
      uint32_t mac :1;
      uint32_t gso :1;
      uint32_t guest_tso4 :1;
      uint32_t guest_tso6 :1;
      uint32_t guest_ecn :1;
      uint32_t guest_ufo :1;
      uint32_t host_tso4 :1;
      uint32_t host_tso6 :1;
      uint32_t host_ecn :1;
      uint32_t host_ufo :1;
      uint32_t mrg_rxbuf :1;
      uint32_t status :1;
      uint32_t ctrl_vq :1;
      uint32_t ctrl_rx :1;
      uint32_t ctrl_vlan :1;
      uint32_t guest_announce :1;
    };
  };

  const size_t VIRTIO_HEADER_LEN = 10;
}

ebbrt::EbbRoot*
ebbrt::VirtioNet::ConstructRoot()
{
  return new SharedRoot<VirtioNet>();
}

ebbrt::VirtioNet::VirtioNet(EbbId id) : Ethernet{id}, next_free_{0},
  next_available_{0}, last_sent_used_{0}, msix_enabled_{false}
{
  auto it = std::find_if(pci->DevicesBegin(), pci->DevicesEnd(),
                         [] (const PCI::Device& d) {
                           return d.VendorId() == 0x1af4 &&
                           d.DeviceId() >= 0x1000 &&
                           d.DeviceId() <= 0x103f &&
                           d.GeneralHeaderType() &&
                           d.SubsystemId() == 1;
                         });

  LRT_ASSERT(it != pci->DevicesEnd());
  uint8_t ptr = it->FindCapability(PCI::Device::CAP_MSIX);
  LRT_ASSERT(ptr != 0);
  it->EnableMsiX(ptr);
  msix_enabled_ = true;
  uint8_t bar = it->MsiXTableBIR(ptr);
  uint32_t msix_addr = it->BAR(bar);
  uint32_t offset = it->MsiXTableOffset(ptr);
  LRT_ASSERT((msix_addr & 1) == 0);
  msix_addr += offset;
  auto msix_table = reinterpret_cast<volatile PCI::Device::MSIXTableEntry*>
    (msix_addr);
  // msix_table[0].raw[0] = 0;
  // msix_table[0].upper = 0xFEE;
  // msix_table[0].raw[1] = 0;
  // msix_table[0].raw[2] = 0;
  // msix_table[0].vector = event_manager->AllocateInterrupt(nullptr);
  // msix_table[0].raw[3] = 0;
  msix_table->raw[0] = 0xFEE00000;
  msix_table->raw[1] = 0;
  msix_table->raw[2] = event_manager->AllocateInterrupt([]() {
      ethernet->SendComplete();
    });
  msix_table->raw[3] = 0;

  memset(empty_header_, 0, VIRTIO_HEADER_LEN);
  io_addr_ = static_cast<uint16_t>(it->BAR(0) & ~0x3);
  it->BusMaster(true);

  virtio::acknowledge(io_addr_);
  virtio::driver(io_addr_);

  // Negotiate features
  Features features;
  features.raw = virtio::device_features(io_addr_);

  Features supported_features;
  supported_features.raw = 0;
  supported_features.mac = features.mac;
  virtio::guest_features(io_addr_, supported_features.raw);

  LRT_ASSERT(features.mac);
  uint16_t addr = io_addr_ + 20;
  if (msix_enabled_) {
    addr += 4;
  }
  for (unsigned i = 0; i < 6; ++i) {
    mac_address_[i] = in8(addr + i);
  }
  //TODO: Initialize Receive

  // Initialize Send
  virtio::queue_select(io_addr_, 1);
  virtio::queue_vector(io_addr_, 0);
  LRT_ASSERT(virtio::queue_vector(io_addr_) != 0xFFFF);
  send_max_ = virtio::queue_size(io_addr_);
  size_t send_size = virtio::qsz_bytes(send_max_);
  void* send_queue = memory_allocator->Memalign(4096, send_size);
  std::memset(send_queue, 0, send_size);
  virtio::queue_address(io_addr_, static_cast<uint32_t>
                        (reinterpret_cast<uintptr_t>(send_queue) >> 12));
  send_descs_ = static_cast<virtio::QueueDescriptor*>(send_queue);
  send_available_ = reinterpret_cast<virtio::Available*>
    (&send_descs_[send_max_]);
  send_used_ = reinterpret_cast<virtio::Used*>
    ((reinterpret_cast<uintptr_t>(&send_available_->ring[send_max_]) + 4095)
     & ~4095);

  // Tell the device we are good to go
  virtio::driver_ok(io_addr_);
}

void
ebbrt::VirtioNet::Send(BufferList buffers,
                       std::function<void()> cb)
{
  size_t size = VIRTIO_HEADER_LEN;
  for (auto& buffer : buffers) {
    size += buffer.second;
  }
  LRT_ASSERT(size <= 1512);

  lock_.Lock();
  LRT_ASSERT((next_free_ + buffers.size() + 1) < send_max_);
  uint16_t desc_index = next_free_;
  next_free_ += buffers.size();
  lock_.Unlock();

  uint16_t index = desc_index;
  send_descs_[index].address = reinterpret_cast<uint64_t>(empty_header_);
  send_descs_[index].length = VIRTIO_HEADER_LEN;
  send_descs_[index].flags.next = true;
  send_descs_[index].next = index + 1;
  ++index;
  for (auto& buffer : buffers) {
    send_descs_[index].address = reinterpret_cast<uint64_t>(buffer.first);
    send_descs_[index].length = buffer.second;
    send_descs_[index].flags.next = true;
    send_descs_[index].next = index + 1;
    ++index;
  }
  send_descs_[index - 1].flags.next = false;

  lock_.Lock();
  send_available_->ring[next_available_] = desc_index;
  std::atomic_thread_fence(std::memory_order_release);
  send_available_->index++;
  if (cb) {
    cb_map_.insert(std::make_pair(desc_index, std::move(cb)));
  }
  lock_.Unlock();

  std::atomic_thread_fence(std::memory_order_release);
  virtio::queue_notify(io_addr_, 1);
}

const uint8_t*
ebbrt::VirtioNet::MacAddress()
{
  return mac_address_;
}

void
ebbrt::VirtioNet::SendComplete()
{
  //This is only ever invoked on core 0 so no synchronization needed
  uint16_t index;
  do {
    index = send_used_->index;
    while(last_sent_used_ != index) {
      auto desc = send_used_->ring[last_sent_used_].index;
      //TODO: actually free the descriptors
      cb_map_[desc]();
      cb_map_.erase(desc);
      last_sent_used_ = (last_sent_used_ + 1) % send_max_;
    }
  } while (index != send_used_->index);
}

void
ebbrt::VirtioNet::Register(uint16_t ethertype,
                           std::function<void(const char*, size_t)> func)
{
  LRT_ASSERT(0);
}

void
ebbrt::VirtioNet::Receive()
{
  LRT_ASSERT(0);
}
