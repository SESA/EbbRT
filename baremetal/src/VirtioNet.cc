//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/VirtioNet.h>

#include <ebbrt/Debug.h>
#include <ebbrt/EventManager.h>

namespace {
const constexpr int kMac = 5;
const constexpr int kMrgRxbuf = 15;
}

ebbrt::VirtioNetDriver::VirtioNetDriver(pci::Device &dev)
    : VirtioDriver<VirtioNetDriver>(dev) {
  std::memset(static_cast<void *>(&empty_header_), 0, sizeof(empty_header_));

  for (int i = 0; i < 6; ++i) {
    mac_addr_[i] = DeviceConfigRead8(i);
  }

  kprintf("Mac Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac_addr_[0],
          mac_addr_[1], mac_addr_[2], mac_addr_[3], mac_addr_[4], mac_addr_[5]);

  FillRxRing();

  auto rcv_vector = event_manager->AllocateVector([this]() {
    auto &rcv_queue = GetQueue(0);
    rcv_queue.ProcessUsedBuffers([this](MutableBufferList list, size_t len) {
      kassert(list.size() == 1);
      list.front() += sizeof(VirtioNetHeader);
      itf_->ReceivePacket(std::move(list.front()),
                          len - sizeof(VirtioNetHeader));
    });
    if (rcv_queue.num_free_descriptors() * 2 >= rcv_queue.Size()) {
      FillRxRing();
    }
  });
  dev.SetMsixEntry(0, rcv_vector, 0);

  auto send_vector = event_manager->AllocateVector([this]() {
    auto &send_queue = GetQueue(1);
    send_queue.CleanUsedBuffers();
  });
  dev.SetMsixEntry(1, send_vector, 0);

  AddDeviceStatus(kConfigDriverOk);

  itf_ = &network_manager->NewInterface(*this);
}

void ebbrt::VirtioNetDriver::FillRxRing() {
  auto &rcv_queue = GetQueue(0);
  auto num_bufs = rcv_queue.num_free_descriptors();
  auto bufs = std::vector<MutableBufferList>(num_bufs);

  for (auto &buf_list : bufs) {
    buf_list.emplace_front(2048);
  }

  auto it = rcv_queue.AddWritableBuffers(bufs.begin(), bufs.end());
  kassert(it == bufs.end());
}

uint32_t ebbrt::VirtioNetDriver::GetDriverFeatures() {
  return 1 << kMac | 1 << kMrgRxbuf;
}

void ebbrt::VirtioNetDriver::Send(ConstBufferList bufs) {
  bufs.emplace_front(static_cast<const void *>(&empty_header_),
                     sizeof(empty_header_));

  auto &send_queue = GetQueue(1);
  kbugon(send_queue.num_free_descriptors() < bufs.size(),
         "Must queue a packet, no more room\n");

  send_queue.AddReadableBuffer(std::move(bufs));
}

const std::array<char, 6> &ebbrt::VirtioNetDriver::GetMacAddress() {
  return mac_addr_;
}
