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
const constexpr int kNotifyOnEmpty = 24;
}

ebbrt::VirtioNetDriver::VirtioNetDriver(pci::Device& dev)
    : VirtioDriver<VirtioNetDriver>(dev) {
  kbugon(!(GetDeviceFeatures() & (1 << 24)),
         "Device does not support notify on empty!\n");
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
    rcv_queue.ProcessUsedBuffers([this](std::unique_ptr<const IOBuf>&& b) {
      kassert(b->CountChainElements() == 1);
      kassert(b->Unique());
      auto buf = std::unique_ptr<IOBuf>(const_cast<IOBuf*>(b.release()));
      buf->Advance(sizeof(VirtioNetHeader));
      itf_->Receive(std::move(buf));
    });
    if (rcv_queue.num_free_descriptors() * 2 >= rcv_queue.Size()) {
      FillRxRing();
    }
  });
  dev.SetMsixEntry(0, rcv_vector, 0);

  auto send_vector =
      event_manager->AllocateVector([this]() { FreeSentPackets(); });
  dev.SetMsixEntry(1, send_vector, 0);

  // We disable the interrupt on packets being sent because we clear everytime
  // send() gets called. Additionally the device will interrupt us if the queue
  // is empty (so we will clear in some bounded time)
  auto& send_queue = GetQueue(1);
  send_queue.DisableAllInterrupts();

  AddDeviceStatus(kConfigDriverOk);

  itf_ = &network_manager->NewInterface(*this);
}

void ebbrt::VirtioNetDriver::FillRxRing() {
  auto& rcv_queue = GetQueue(0);
  auto num_bufs = rcv_queue.num_free_descriptors();
  auto bufs = std::vector<std::unique_ptr<IOBuf>>();
  bufs.reserve(num_bufs);

  for (size_t i = 0; i < num_bufs; ++i) {
    bufs.emplace_back(IOBuf::Create(2048));
  }

  auto it = rcv_queue.AddWritableBuffers(bufs.begin(), bufs.end());
  kassert(it == bufs.end());
}

void ebbrt::VirtioNetDriver::FreeSentPackets() {
  auto& send_queue = GetQueue(1);
  // Do nothing, just free the buffer
  send_queue.ProcessUsedBuffers([](std::unique_ptr<const IOBuf>&& b) {});
  while (!packet_queue_.empty() &&
         send_queue.num_free_descriptors() >=
             packet_queue_.front()->CountChainElements()) {
    send_queue.AddReadableBuffer(std::move(packet_queue_.front()));
    packet_queue_.pop();
  }
}

uint32_t ebbrt::VirtioNetDriver::GetDriverFeatures() {
  return 1 << kMac | 1 << kMrgRxbuf | 1 << kNotifyOnEmpty;
}

void ebbrt::VirtioNetDriver::Send(std::unique_ptr<const IOBuf>&& buf) {
  FreeSentPackets();
  auto b = IOBuf::WrapBuffer(static_cast<void*>(&empty_header_),
                             sizeof(empty_header_));
  // const cast is OK because we then take the head of the chain as const
  b->AppendChain(std::unique_ptr<IOBuf>(const_cast<IOBuf*>(buf.release())));

  auto& send_queue = GetQueue(1);
  if (send_queue.num_free_descriptors() <
      b->CountChainElements()) {  // Must queue a packet, no more room
    packet_queue_.emplace(std::move(b));
    return;
  }

  send_queue.AddReadableBuffer(std::move(b));
}

const std::array<char, 6>& ebbrt::VirtioNetDriver::GetMacAddress() {
  return mac_addr_;
}
