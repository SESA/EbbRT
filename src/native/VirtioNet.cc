//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "VirtioNet.h"

#include "../StaticIOBuf.h"
#include "../UniqueIOBuf.h"
#include "Debug.h"
#include "EventManager.h"

namespace {
const constexpr uint32_t kCSum = 0;
const constexpr uint32_t kGuestCSum = 1;
const constexpr uint32_t kMac = 5;
const constexpr uint32_t kGuestTso4 = 7;
const constexpr uint32_t kGuestTso6 = 8;
const constexpr uint32_t kGuestEcn = 9;
const constexpr uint32_t kGuestUfo = 10;
const constexpr uint32_t kHostTso4 = 11;
const constexpr uint32_t kHostTso6 = 12;
const constexpr uint32_t kHostEcn = 13;
const constexpr uint32_t kHostUfo = 14;
const constexpr uint32_t kMrgRxbuf = 15;
const constexpr uint32_t kCtrlVq = 17;
const constexpr uint32_t kMq = 22;
const constexpr uint32_t kNotifyOnEmpty = 24;

const constexpr uint8_t kVirtioNetCtrlMq = 4;
const constexpr uint8_t kVirtioNetCtrlMqVqPairsSet = 0;

struct VirtioNetCtrlHeader {
  uint8_t cls;
  uint8_t command;
} __attribute__((packed));

struct VirtioNetCtrlMq {
  uint16_t virtqueue_pairs;
};

typedef uint8_t VirtioNetCtrlAck;
const constexpr uint8_t kVirtioNetOk = 0;
const constexpr uint8_t kVirtioNetErr = 1;
}  // namespace

void ebbrt::VirtioNetDriver::Create(pci::Device& dev) {
  auto virt_dev = new VirtioNetDriver(dev);
  virt_dev->ebb_ =
      VirtioNetRep::Create(virt_dev, ebb_allocator->AllocateLocal());

  // Start interrupts after ebb_ has been created
  // else could encounter race condition
  // where we try to access ebb_ when interrupt fires
  // and it hasn't been created yet
  virt_dev->Start();
}

ebbrt::VirtioNetDriver::VirtioNetDriver(pci::Device& dev)
    : VirtioDriver<VirtioNetDriver>(dev),
      itf_(network_manager->NewInterface(*this)) {
  auto features = SetupFeatures();
  auto multiqueue = features & (1 << kMq);
  kbugon(!multiqueue, "Device missing multiqueue support!\n");
  auto csum = features & (1 << kCSum);
  kbugon(!csum, "Device missing checksum offloading support!\n");
  auto tso4 = features & (1 << kHostTso4);
  kbugon(!tso4, "Device missing tcp segmentation offload support\n");

  // Figure out max queue pairs supported
  auto max_queue_pairs = DeviceConfigRead16(8);
  auto num_cores = Cpu::Count();
  // We map a queue pair to each core, so assert that there are enough
  kassert(max_queue_pairs >= num_cores);
  size_t used_queue_pairs = num_cores;
  SetNumQueues(max_queue_pairs * 2 + 1);

  // Tell the device how many queues we will use
  // Initialize control queue
  ctrl_queue_ = &InitializeQueue(max_queue_pairs * 2);

  // Write number of queue pairs to be used
  // Set Header
  auto buf = MakeUniqueIOBuf(sizeof(VirtioNetCtrlHeader));
  auto buf_dp = buf->GetMutDataPointer();
  auto& ctrl = buf_dp.Get<VirtioNetCtrlHeader>();
  ctrl.cls = kVirtioNetCtrlMq;
  ctrl.command = kVirtioNetCtrlMqVqPairsSet;

  // Set MQ command
  auto cmd_buf = MakeUniqueIOBuf(sizeof(VirtioNetCtrlMq));
  auto cmd_dp = cmd_buf->GetMutDataPointer();
  auto& s = cmd_dp.Get<VirtioNetCtrlMq>();
  s.virtqueue_pairs = used_queue_pairs;
  buf->PrependChain(std::move(cmd_buf));

  // Allocate status buffer
  auto status_buf = MakeUniqueIOBuf(sizeof(VirtioNetCtrlAck));
  auto status_dp = status_buf->GetDataPointer();
  const auto& status = status_dp.Get<VirtioNetCtrlAck>();
  buf->PrependChain(std::move(status_buf));

  ctrl_queue_->AddBuffer(std::move(buf), 2);
  // poll control queue until complete
  while (!ctrl_queue_->HasUsedBuffer()) {
    ctrl_queue_->Kick();
  }
  ctrl_queue_->ClearUsedBuffers();
  kassert(status == kVirtioNetOk);

  kprintf("Negotiated %d queue pairs\n", used_queue_pairs);

  auto rcv_vector =
      event_manager->AllocateVector([this]() { ebb_->Receive(); });

  for (size_t i = 0; i < used_queue_pairs; ++i) {
    auto& rcv_queue = InitializeQueue(i * 2, Cpu::GetByIndex(i)->nid());
    auto& snd_queue = InitializeQueue(i * 2 + 1, Cpu::GetByIndex(i)->nid());

    // Fill receive queue
    auto num_bufs = rcv_queue.num_free_descriptors();
    auto bufs = std::vector<std::unique_ptr<MutIOBuf>>();
    bufs.reserve(num_bufs);

    for (size_t i = 0; i < num_bufs; ++i) {
      bufs.emplace_back(MakeUniqueIOBuf(65562));
    }

    auto it = rcv_queue.AddWritableBuffers(bufs.begin(), bufs.end());
    kassert(it == bufs.end());

    dev.SetMsixEntry(i * 2, rcv_vector, i);
    // We disable the interrupt on packets being sent because we clear everytime
    // send() gets called. Additionally the device will interrupt us if the
    // queue
    // is empty (so we will clear in some bounded time)
    snd_queue.DisableInterrupts();
  }

  for (int i = 0; i < 6; ++i) {
    mac_addr_[i] = DeviceConfigRead8(i);
  }

  kprintf(
      "Mac Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
      static_cast<uint8_t>(mac_addr_[0]), static_cast<uint8_t>(mac_addr_[1]),
      static_cast<uint8_t>(mac_addr_[2]), static_cast<uint8_t>(mac_addr_[3]),
      static_cast<uint8_t>(mac_addr_[4]), static_cast<uint8_t>(mac_addr_[5]));
}

void ebbrt::VirtioNetDriver::Start() {
  // enables interrupts to process packets
  AddDeviceStatus(kConfigDriverOk);
}

uint32_t ebbrt::VirtioNetDriver::GetDriverFeatures() {
  return 1 << kCSum | 1 << kGuestCSum | 1 << kMac | 1 << kGuestTso4 |
         1 << kGuestUfo | 1 << kHostTso4 | 1 << kHostUfo | 1 << kMrgRxbuf |
         1 << kCtrlVq | 1 << kMq;
}

ebbrt::VirtioNetRep::VirtioNetRep(const VirtioNetDriver& root)
    : root_(root), rcv_queue_(root_.GetQueue(Cpu::GetMine() * 2)),
      snd_queue_(root_.GetQueue(Cpu::GetMine() * 2 + 1)),
      receive_callback_([this]() { ReceivePoll(); }), circ_buffer_head_(0),
      circ_buffer_tail_(0) {}

void ebbrt::VirtioNetDriver::Send(std::unique_ptr<IOBuf> buf,
                                  PacketInfo pinfo) {
  ebb_->Send(std::move(buf), std::move(pinfo));
}

void ebbrt::VirtioNetRep::Send(std::unique_ptr<IOBuf> buf, PacketInfo pinfo) {
  std::unique_ptr<MutUniqueIOBuf> b;

  snd_queue_.ClearUsedBuffers();
  VirtioNetHeader* header;
  auto free_desc = snd_queue_.num_free_descriptors();
#ifdef VIRTIO_ZERO_COPY
  if (free_desc > buf->CountChainElements()) {
    // we have enough descriptors to avoid a copy
    b = MakeUniqueIOBuf(sizeof(VirtioNetHeader), /* zero_memory = */ true);
    header = reinterpret_cast<VirtioNetHeader*>(b->MutData());
    b->PrependChain(std::move(buf));
  } else  // NOLINT
#endif
      if (free_desc >= 1) {
    // XXX: Maybe we should use indirect descriptors instead?
    // copy into one buffer
    auto len = buf->ComputeChainDataLength();
    b = MakeUniqueIOBuf(len + sizeof(VirtioNetHeader));
    memset(b->MutData(), 0, sizeof(VirtioNetHeader));
    header = reinterpret_cast<VirtioNetHeader*>(b->MutData());
    auto data = b->MutData() + sizeof(VirtioNetHeader);
    for (auto& buf_it : *buf) {
      memcpy(data, buf_it.Data(), buf_it.Length());
      data += buf_it.Length();
    }
  } else {
    kprintf("Drop\n");
    // kick to make it process more buffers, drop the send
    snd_queue_.Kick();
    return;
  }

  kassert(header != nullptr);
  if (pinfo.flags & PacketInfo::kNeedsCsum) {
    header->flags |= VirtioNetHeader::kNeedsCsum;
    header->csum_start = pinfo.csum_start;
    header->csum_offset = pinfo.csum_offset;
  }
  if (pinfo.gso_type != PacketInfo::kGsoNone) {
    header->gso_type = pinfo.gso_type;
    header->hdr_len = pinfo.hdr_len;
    header->gso_size = pinfo.gso_size;
  }
  auto elements = b->CountChainElements();
  snd_queue_.AddBuffer(std::move(b), elements);
}

const ebbrt::EthernetAddress& ebbrt::VirtioNetDriver::GetMacAddress() {
  return mac_addr_;
}

void ebbrt::VirtioNetRep::Receive() {
  rcv_queue_.DisableInterrupts();
  receive_callback_.Start();
}

void ebbrt::VirtioNetRep::ReceivePoll() {
#ifndef VIRTIO_NET_POLL
process:
#endif
  rcv_queue_.ProcessUsedBuffers([this](std::unique_ptr<MutIOBuf> buf) {
    circ_buffer_[circ_buffer_head_ % 256] = std::move(buf);
    ++circ_buffer_head_;
    if (circ_buffer_head_ != circ_buffer_tail_ &&
        (circ_buffer_head_ % 256) == (circ_buffer_tail_ % 256))
      ++circ_buffer_tail_;
  });
  // If there are no used buffers, turn on interrupts and stop this poll
  if (circ_buffer_head_ == circ_buffer_tail_) {
#ifdef VIRTIO_NET_POLL
    return;
#else
    rcv_queue_.EnableInterrupts();
    // Double check to avoid race
    if (likely(!rcv_queue_.HasUsedBuffer())) {
      receive_callback_.Stop();
      return;
    } else {
      // raced, disable interrupts
      rcv_queue_.DisableInterrupts();
      goto process;
    }
#endif
  }

  kassert(circ_buffer_head_ != circ_buffer_tail_);
  kassert(circ_buffer_[circ_buffer_tail_ % 256]);
  auto b = std::move(circ_buffer_[circ_buffer_tail_ % 256]);
  ++circ_buffer_tail_;

  if (rcv_queue_.num_free_descriptors() * 2 >= rcv_queue_.Size()) {
    FillRxRing();
  }

  kassert(b->CountChainElements() == 1);
  // auto header = reinterpret_cast<VirtioNetHeader*>(b->MutData());
  // if (header->flags & VirtioNetHeader::kNeedsCsum) {

  // }
  b->Advance(sizeof(VirtioNetHeader));
  root_.itf_.Receive(std::move(b));
}

void ebbrt::VirtioNetRep::FillRxRing() {
  auto num_bufs = rcv_queue_.num_free_descriptors();
  auto bufs = std::vector<std::unique_ptr<MutIOBuf>>();
  bufs.reserve(num_bufs);

  for (size_t i = 0; i < num_bufs; ++i) {
    bufs.emplace_back(MakeUniqueIOBuf(65562));
  }

  auto it = rcv_queue_.AddWritableBuffers(bufs.begin(), bufs.end());
  kassert(it == bufs.end());
}
