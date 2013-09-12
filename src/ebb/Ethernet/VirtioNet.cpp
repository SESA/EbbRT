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

#include "arch/inet.hpp"
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

ebbrt::VirtioNet::VirtioNet(EbbId id) : Ethernet{id}
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

  //Setup receive interrupt
  msix_table[0].raw[0] = 0xFEE00000;
  msix_table[0].raw[1] = 0;
  msix_table[0].raw[2] = event_manager->AllocateInterrupt([]() {
      ethernet->Receive();
    });
  msix_table[0].raw[3] = 0;

  //Setup send complete interrupt
  msix_table[1].raw[0] = 0xFEE00000;
  msix_table[1].raw[1] = 0;
  msix_table[1].raw[2] = event_manager->AllocateInterrupt([]() {
      ethernet->SendComplete();
    });
  msix_table[1].raw[3] = 0;

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
  uint16_t addr = io_addr_ + 24;

  for (unsigned i = 0; i < 6; ++i) {
    mac_address_[i] = in8(addr + i);
  }

  // Initialize Receive
  virtio::queue_select(io_addr_, 0);
  virtio::queue_vector(io_addr_, 0);
  LRT_ASSERT(virtio::queue_vector(io_addr_) != 0xFFFF);
  InitRing(receive_);

  // Initialize Send
  virtio::queue_select(io_addr_, 1);
  virtio::queue_vector(io_addr_, 1);
  LRT_ASSERT(virtio::queue_vector(io_addr_) != 0xFFFF);
  InitRing(send_);

  std::atomic_thread_fence(std::memory_order_release);

  // Tell the device we are good to go
  virtio::driver_ok(io_addr_);

  // Add buffers to receive queue
  for (uint16_t i = 0; i < receive_.size; ++i) {
    receive_.descs[i].address =
      reinterpret_cast<uintptr_t>(new char[1514]);
    receive_.descs[i].length = 1514;
    receive_.descs[i].flags.write = true;
    receive_.available->ring[i] = i;
  }
  std::atomic_thread_fence(std::memory_order_release);
  receive_.available->index = receive_.size - 1;
  std::atomic_thread_fence(std::memory_order_release);
  virtio::queue_notify(io_addr_, 0);
}

ebbrt::Buffer
ebbrt::VirtioNet::Alloc(size_t size)
{
  auto mem = std::malloc(size + 14);
  if (mem == nullptr) {
    throw std::bad_alloc();
  }
  return (Buffer(mem, size + 14) + 14);
}

void
ebbrt::VirtioNet::Send(Buffer buffer, const char* to,
                       const char* from, uint16_t ethertype)
{
  auto buf = buffer - 14;
  auto data = buf.data();
  memcpy(data, to, 6);
  memcpy(data + 6, from, 6);
  *reinterpret_cast<uint16_t*>(data + 12) = htons(ethertype);
  //FIXME: handle this by queueing the buffer to be sent out later
  LRT_ASSERT(send_.num_free > 2);
  send_.num_free -= 2;
  uint16_t head = send_.free_head;

  uint16_t index = head;
  send_.descs[index].address = reinterpret_cast<uint64_t>(empty_header_);
  send_.descs[index].length = VIRTIO_HEADER_LEN;
  send_.descs[index].flags.next = true;
  send_.descs[index].next = index + 1;
  ++index;
  send_.descs[index].address = reinterpret_cast<uint64_t>(data);
  send_.descs[index].length = buf.length();
  send_.descs[index].flags.next = false;

  auto avail = send_.available->index % send_.size;
  send_.available->ring[avail] = head;

  //Make the descriptor and available array update visible before
  // giving it to the device
  std::atomic_thread_fence(std::memory_order_release);

  send_.available->index++;

  std::atomic_thread_fence(std::memory_order_release);
  virtio::queue_notify(io_addr_, 1);

  buffer_map_.emplace(head, std::move(buffer));
}

const char*
ebbrt::VirtioNet::MacAddress()
{
  return mac_address_;
}

void
ebbrt::VirtioNet::SendComplete()
{
  uint16_t index;
  index = send_.used->index;
  while(send_.last_used != index) {
    auto last_used = send_.last_used % send_.size;
    auto desc = send_.used->ring[last_used].index;
    int freed = 1;
    auto i = desc;
    while (send_.descs[i].flags.next) {
      i = send_.descs[i].next;
      freed++;
    }
    send_.descs[i].next = send_.free_head;
    send_.free_head = desc;
    send_.num_free += freed;

    buffer_map_.erase(desc);
    send_.last_used++;
  }
}

void
ebbrt::VirtioNet::Register(uint16_t ethertype,
                           std::function<void(Buffer, const char[6])> func)
{
  rcv_map_[ethertype] = func;
}

void
ebbrt::VirtioNet::Receive()
{
  LRT_ASSERT(0);
  // //FIXME: temporarily disable interrupts on the device as an optimization
  // int avail_index = receive_.available->index;
  // auto used_index = receive_.used->index;
  // while (receive_.last_used != used_index) {
  //   auto last_used = receive_.last_used % receive_.size;
  //   auto desc = receive_.used->ring[last_used].index;

  //   auto buf = reinterpret_cast<const char*>(receive_.descs[desc].address);
  //   uint16_t ethertype =
  //     ntohs(*reinterpret_cast<const uint16_t*>(&buf[VIRTIO_HEADER_LEN + 12]));
  //   auto it = rcv_map_.find(ethertype);
  //   if (it != rcv_map_.end()) {
  //     //If we have a listener registered then invoke them
  //     it->second()
  //   }
  // }
}

void
ebbrt::VirtioNet::InitRing(Ring& ring)
{
  ring.size = virtio::queue_size(io_addr_);
  size_t bytes = virtio::qsz_bytes(ring.size);
  void* queue = memory_allocator->Memalign(4096, bytes);
  if (queue == nullptr) {
    throw std::bad_alloc();
  }
  std::memset(queue, 0, bytes);
  virtio::queue_address(io_addr_, static_cast<uint32_t>
                        (reinterpret_cast<uintptr_t>(queue) >> 12));
  ring.descs = static_cast<virtio::QueueDescriptor*>(queue);
  for (unsigned i = 0; i < ring.size; ++i) {
    ring.descs[i].next = i + 1;
  }
  ring.available = reinterpret_cast<virtio::Available*>
    (&ring.descs[ring.size]);
  ring.used = reinterpret_cast<virtio::Used*>
    ((reinterpret_cast<uintptr_t>(&ring.available->ring[ring.size]) + 4095)
     & ~4095);
  ring.free_head = 0;
  ring.num_free = ring.size;
  ring.last_used = 0;
}
