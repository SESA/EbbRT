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

void
ebbrt::VirtioNet::InitRing(Ring& ring)
{
  ring.size = virtio::queue_size(io_addr_);
  size_t bytes = virtio::qsz_bytes(ring.size);
  void* queue = memory_allocator->Memalign(4096, bytes);
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

ebbrt::VirtioNet::VirtioNet()
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
  uint8_t bar = it->MsiXTableBIR(ptr);
  uint32_t msix_addr = it->BAR(bar);
  uint32_t offset = it->MsiXTableOffset(ptr);
  LRT_ASSERT((msix_addr & 1) == 0);
  msix_addr += offset;
  auto msix_table = reinterpret_cast<volatile PCI::Device::MSIXTableEntry*>
    (msix_addr);
  bar = it->MsiXPBABIR(ptr);
  uint32_t pba_addr = it->BAR(bar);
  offset = it->MsiXPBAOffset(ptr);
  LRT_ASSERT((pba_addr & 1) == 0);
  pba_addr += offset;
  msix_table[0].raw[0] = 0xFEE00000;
  msix_table[0].raw[1] = 0;
  msix_table[0].raw[2] = event_manager->AllocateInterrupt([]() {
      ethernet->Receive();
    });
  msix_table[0].raw[3] = 0;
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

  // Read MAC address
  LRT_ASSERT(features.mac);
  uint16_t addr = io_addr_ + 24; //from spec, this is where the header
                                 //ends if msix is enabled
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

  // Tell the device we are good to go
  virtio::driver_ok(io_addr_);

  // Add buffers to the receive queue
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

void
ebbrt::VirtioNet::Send(BufferList buffers,
                       std::function<void()> cb)
{
  size_t size = VIRTIO_HEADER_LEN;
  for (auto& buffer : buffers) {
    size += buffer.second;
  }
  LRT_ASSERT(size <= 1514);

  send_.lock.Lock();
  // Currently we don't handle the case when we have not enough free
  // descriptors
  LRT_ASSERT((buffers.size() + 1) < send_.num_free);
  send_.num_free -= buffers.size() + 1;
  uint16_t head = send_.free_head;

  if (cb) {
    cb_map_lock_.Lock();
    cb_map_.insert(std::make_pair(head, std::move(cb)));
    cb_map_lock_.Unlock();
  }

  uint16_t prev;
  uint16_t index = head;
  send_.descs[index].address = reinterpret_cast<uint64_t>(empty_header_);
  send_.descs[index].length = VIRTIO_HEADER_LEN;
  send_.descs[index].flags.next = true;
  index = send_.descs[index].next;
  for (const auto& buffer : buffers) {
    send_.descs[index].address = reinterpret_cast<uint64_t>(buffer.first);
    send_.descs[index].length = buffer.second;
    send_.descs[index].flags.next = true;
    prev = index;
    index = send_.descs[index].next;
  }
  send_.descs[prev].flags.next = false;
  send_.free_head = index;

  auto avail = send_.available->index % send_.size;
  send_.available->ring[avail] = head;

  //Make the descriptor and available array update visible before
  //giving it to the device
  std::atomic_thread_fence(std::memory_order_release);

  send_.available->index++;
  send_.lock.Unlock();

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
  auto index = send_.used->index;
  while (send_.last_used != index) {
    auto last_used = send_.last_used % send_.size;
    auto desc = send_.used->ring[last_used].index;
    int freed = 1;
    auto i = desc;
    while (send_.descs[i].flags.next) {
      i = send_.descs[i].next;
      freed++;
    }
    send_.lock.Lock();
    send_.descs[i].next = send_.free_head;
    send_.free_head = desc;
    send_.num_free += freed;
    send_.lock.Unlock();
    cb_map_lock_.Lock();
    auto it = cb_map_.find(desc);
    if (it != cb_map_.end()) {
      auto f = it->second;
      cb_map_.erase(desc);
      cb_map_lock_.Unlock();
      f();
    } else {
      cb_map_lock_.Unlock();
    }
    send_.last_used++;
  }
}

void
ebbrt::VirtioNet::Register(uint16_t ethertype,
                           std::function<void(const char*, size_t)> func)
{
  rcv_map_[ethertype] = func;
}

void
ebbrt::VirtioNet::Receive()
{
  //FIXME: temporarily disable interrupts as an optimization
  int avail_index = receive_.available->index;
  auto used_index = receive_.used->index;
  while (receive_.last_used != used_index) {
    auto last_used = receive_.last_used % receive_.size;
    auto desc = receive_.used->ring[last_used].index;

    auto buf = reinterpret_cast<const char*>(receive_.descs[desc].address);
    uint16_t ethertype =
      ntohs(*reinterpret_cast<const uint16_t*>(&buf[VIRTIO_HEADER_LEN + 12]));
    auto it = rcv_map_.find(ethertype);
    if (it != rcv_map_.end()) {
      it->second(&buf[VIRTIO_HEADER_LEN],
                 receive_.used->ring[last_used].length - VIRTIO_HEADER_LEN);
    }
    receive_.available->ring[avail_index % receive_.size] = desc;
    avail_index++;
    receive_.last_used++;
  }
  std::atomic_thread_fence(std::memory_order_release);
  receive_.available->index = avail_index;
  std::atomic_thread_fence(std::memory_order_release);
  virtio::queue_notify(io_addr_, 0);
}
