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
    : VirtioDriver<VirtioNetDriver>(dev),
      receive_callback_([this]() { ReceivePoll(); }) {
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
    rcv_queue.DisableInterrupts();
    receive_callback_.Start();
  });
  dev.SetMsixEntry(0, rcv_vector, 0);

  // We disable the interrupt on packets being sent because we clear everytime
  // send() gets called. Additionally the device will interrupt us if the queue
  // is empty (so we will clear in some bounded time)
  auto& send_queue = GetQueue(1);
  send_queue.DisableInterrupts();

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
  send_queue.ClearUsedBuffers();
}

uint32_t ebbrt::VirtioNetDriver::GetDriverFeatures() {
  return 1 << kMac | 1 << kMrgRxbuf;
}

void ebbrt::VirtioNetDriver::Send(std::unique_ptr<const IOBuf>&& buf) {
#if 0
  auto b = IOBuf::WrapBuffer(static_cast<void*>(&empty_header_),
                             sizeof(empty_header_));
  // const cast is OK because we then take the head of the chain as const
  b->AppendChain(std::unique_ptr<IOBuf>(const_cast<IOBuf*>(buf.release())));
#else
  auto b = IOBuf::Create(buf->ComputeChainDataLength() + sizeof(empty_header_));
  auto data = b->WritableData();
  memcpy(data, static_cast<void*>(&empty_header_), sizeof(empty_header_));
  data += sizeof(empty_header_);
  for (auto& buf_it : *buf) {
    memcpy(data, buf_it.Data(), buf_it.Length());
    data += buf_it.Length();
  }
#endif

  auto& send_queue = GetQueue(1);
  if (unlikely(send_queue.num_free_descriptors() < b->CountChainElements())) {
    FreeSentPackets();
    if (unlikely(send_queue.num_free_descriptors() < b->CountChainElements())) {
      return;  // Drop
    }
  }

  send_queue.AddReadableBuffer(std::move(b));
}

const std::array<char, 6>& ebbrt::VirtioNetDriver::GetMacAddress() {
  return mac_addr_;
}

void ebbrt::VirtioNetDriver::Poll() { FreeSentPackets(); }

void ebbrt::VirtioNetDriver::ReceivePoll() {
  auto& rcv_queue = GetQueue(0);
  // If there are no used buffers, turn on interrupts and stop this poll
  if (!rcv_queue.HasUsedBuffer()) {
    rcv_queue.EnableInterrupts();
    // Double check to avoid race
    if (likely(!rcv_queue.HasUsedBuffer())) {
      receive_callback_.Stop();
      return;
    } else {
      // raced, disable interrupts
      rcv_queue.DisableInterrupts();
    }
  }

  kassert(rcv_queue.HasUsedBuffer());
  auto b = rcv_queue.GetBuffer();
  kassert(b->CountChainElements() == 1);
  kassert(b->Unique());
  b->Advance(sizeof(VirtioNetHeader));

  if (rcv_queue.num_free_descriptors() * 2 >= rcv_queue.Size()) {
    FillRxRing();
  }

  itf_->Receive(std::move(b));
}
