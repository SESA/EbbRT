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

ebbrt::VirtioNetDriver::VirtioNetDriver(pci::Device& dev)
    : VirtioDriver<VirtioNetDriver>(dev) {
  std::memset(static_cast<void*>(&empty_header_), 0, sizeof(empty_header_));

  for (int i = 0; i < 6; ++i) {
    mac_addr_[i] = DeviceConfigRead8(i);
  }

  kprintf(
      "Mac Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
      static_cast<uint8_t>(mac_addr_[0]), static_cast<uint8_t>(mac_addr_[1]),
      static_cast<uint8_t>(mac_addr_[2]), static_cast<uint8_t>(mac_addr_[3]),
      static_cast<uint8_t>(mac_addr_[4]), static_cast<uint8_t>(mac_addr_[5]));

  FillRxRing();

  auto rcv_vector = event_manager->AllocateVector([this]() {
    auto& rcv_queue = GetQueue(0);
    rcv_queue.ProcessUsedBuffers([this](Buffer b) {
      kassert(b.length() == 1);
      b += sizeof(VirtioNetHeader);
      itf_->Receive(std::move(b));
    });
    if (rcv_queue.num_free_descriptors() * 2 >= rcv_queue.Size()) {
      FillRxRing();
    }
  });
  dev.SetMsixEntry(0, rcv_vector, 0);

  auto send_vector = event_manager->AllocateVector([this]() {
    auto& send_queue = GetQueue(1);
    send_queue.CleanUsedBuffers();
  });
  dev.SetMsixEntry(1, send_vector, 0);

  AddDeviceStatus(kConfigDriverOk);

  itf_ = &network_manager->NewInterface(*this);
}

void ebbrt::VirtioNetDriver::FillRxRing() {
  auto& rcv_queue = GetQueue(0);
  auto num_bufs = rcv_queue.num_free_descriptors();
  auto bufs = std::vector<Buffer>();
  bufs.reserve(num_bufs);

  for (size_t i = 0; i < num_bufs; ++i) {
    bufs.emplace_back(2048);
  }

  auto it = rcv_queue.AddWritableBuffers(bufs.begin(), bufs.end());
  kassert(it == bufs.end());
}

uint32_t ebbrt::VirtioNetDriver::GetDriverFeatures() {
  return 1 << kMac | 1 << kMrgRxbuf;
}

void ebbrt::VirtioNetDriver::Send(std::shared_ptr<const Buffer> buf) {
  auto b = std::make_shared<Buffer>(static_cast<void*>(&empty_header_),
                                    sizeof(empty_header_));
  b->append(std::move(std::const_pointer_cast<Buffer>(buf)));

  auto& send_queue = GetQueue(1);
  kbugon(send_queue.num_free_descriptors() < b->length(),
         "Must queue a packet, no more room\n");

  send_queue.AddReadableBuffer(std::move(b));
}

const std::array<char, 6>& ebbrt::VirtioNetDriver::GetMacAddress() {
  return mac_addr_;
}
