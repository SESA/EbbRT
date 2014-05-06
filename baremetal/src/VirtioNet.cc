//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/VirtioNet.h>

#include "lwip/ip.h"
#include "lwip/inet_chksum.h"

#include <ebbrt/Debug.h>
#include <ebbrt/EventManager.h>

namespace {
const constexpr int kCSum = 0;
const constexpr int kGuestCSum = 1;
const constexpr int kMac = 5;
const constexpr int kMrgRxbuf = 15;
const constexpr int kNotifyOnEmpty = 24;
}

ebbrt::VirtioNetDriver::VirtioNetDriver(pci::Device& dev)
    : VirtioDriver<VirtioNetDriver>(dev),
      receive_callback_([this]() { ReceivePoll(); }), circ_buffer_head_(0),
      circ_buffer_tail_(0) {
  auto feat = GetGuestFeatures();
  csum_ = feat & (1 << kCSum);
  if (csum_) {
    kprintf("VirtioNet: csum\n");
  }
  guest_csum_ = feat & (1 << kGuestCSum);
  if (guest_csum_) {
    kprintf("VirtioNet: guest_csum\n");
  }
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
  return 1 << kCSum | 1 << kGuestCSum | 1 << kMac | 1 << kMrgRxbuf;
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

namespace {
struct ethernet_header {
  uint8_t dest_addr[6];
  uint8_t source_addr[6];
  uint16_t ethertype;
};

const constexpr uint16_t kEtherTypeIP = 0x0800;
const constexpr uint16_t kEtherTypeVLan = 0x8100;

u16_t lwip_standard_chksum(const void* dataptr, int len) {
  const uint8_t* pb = reinterpret_cast<const uint8_t*>(dataptr);
  uint16_t t = 0;
  u32_t sum = 0;
  int odd = ((mem_ptr_t)pb & 1);

  /* Get aligned to u16_t */
  if (odd && len > 0) {
    (reinterpret_cast<uint8_t*>(&t))[1] = *pb++;
    len--;
  }

  /* Add the bulk of the data */
  auto ps = reinterpret_cast<const uint16_t*>(pb);
  while (len > 1) {
    sum += *ps++;
    len -= 2;
  }

  /* Consume left-over byte, if any */
  if (len > 0) {
    (reinterpret_cast<uint8_t*>(&t))[0] = *reinterpret_cast<const uint8_t*>(ps);
  }

  /* Add end bytes */
  sum += t;

  /* Fold 32-bit sum to 16 bits
     calling this twice is propably faster than if statements... */
  sum = FOLD_U32T(sum);
  sum = FOLD_U32T(sum);

  /* Swap if alignment was odd */
  if (odd) {
    sum = SWAP_BYTES_IN_WORD(sum);
  }

  return (u16_t)sum;
}

uint16_t inet_chksum_pseudo(const std::unique_ptr<ebbrt::IOBuf>& buf,
                            ip_addr_t* src, ip_addr_t* dest, uint8_t proto,
                            uint16_t proto_len) {
  uint32_t acc = 0;
  uint8_t swapped = 0;

  for (auto& b : *buf) {
    acc += lwip_standard_chksum(b.Data(), b.Length());
    acc = FOLD_U32T(acc);
    if (b.Length() % 2 != 0) {
      swapped = 1 - swapped;
      acc = SWAP_BYTES_IN_WORD(acc);
    }
  }

  if (swapped) {
    acc = SWAP_BYTES_IN_WORD(acc);
  }
  uint32_t addr = ip4_addr_get_u32(src);
  acc += (addr & 0xffffUL);
  acc += ((addr >> 16) & 0xffffUL);
  addr = ip4_addr_get_u32(dest);
  acc += (addr & 0xffffUL);
  acc += ((addr >> 16) & 0xffffUL);
  acc += (u32_t)htons((u16_t)proto);
  acc += (u32_t)htons(proto_len);

  /* Fold 32-bit sum to 16 bits
     calling this twice is propably faster than if statements... */
  acc = FOLD_U32T(acc);
  acc = FOLD_U32T(acc);
  return (uint16_t) ~(acc & 0xffffUL);
}

bool rx_csum(const std::unique_ptr<ebbrt::IOBuf>& buf) {
  auto dp = buf->GetDataPointer();
  auto& eh = dp.Get<ethernet_header>();
  auto eth_type = ntohs(eh.ethertype);
  ebbrt::kbugon(eth_type == kEtherTypeVLan, "VLAN csum not supported\n");
  if (eth_type != kEtherTypeIP) {
    return true;
  }

  auto& ih = dp.GetNoAdvance<ip_hdr>();
  auto iphdr_hlen = IPH_HL(&ih) * 4;
  if (inet_chksum(const_cast<void*>(static_cast<const void*>(&ih)),
                  iphdr_hlen) != 0) {
    ebbrt::kprintf("ip chksum fail\n");
    return false;
  }
  dp.Advance(iphdr_hlen);

  kassert(buf->CountChainElements() == 1);
  buf->Advance(sizeof(ethernet_header) + iphdr_hlen);
  ip_addr_t src;
  ip_addr_copy(src, ih.src);
  ip_addr_t dest;
  ip_addr_copy(dest, ih.dest);
  bool ret = true;
  switch (IPH_PROTO(&ih)) {
  case IP_PROTO_UDP: {
    // handle only single element chain for now
    if (inet_chksum_pseudo(buf, &src, &dest, IP_PROTO_UDP,
                           buf->ComputeChainDataLength()) != 0) {
      ebbrt::kprintf("udp chksum fail\n");
      ret = false;
    }
    break;
  }
  case IP_PROTO_TCP: {
    if (inet_chksum_pseudo(buf, &src, &dest, IP_PROTO_TCP,
                           buf->ComputeChainDataLength()) != 0) {
      ebbrt::kprintf("tcp chksum fail\n");
      ret = false;
    }
    break;
  }
  }
  buf->Retreat(sizeof(ethernet_header) + iphdr_hlen);
  return ret;
}
}  // namespace

void ebbrt::VirtioNetDriver::ReceivePoll() {
  auto& rcv_queue = GetQueue(0);
process:
  rcv_queue.ProcessUsedBuffers([this](std::unique_ptr<IOBuf>&& buf) {
    circ_buffer_[circ_buffer_head_ % 256] = std::move(buf);
    ++circ_buffer_head_;
    if (circ_buffer_head_ != circ_buffer_tail_ &&
        (circ_buffer_head_ % 256) == (circ_buffer_tail_ % 256))
      ++circ_buffer_tail_;
  });
  // If there are no used buffers, turn on interrupts and stop this poll
  if (circ_buffer_head_ == circ_buffer_tail_) {
    // if (!rcv_queue.HasUsedBuffer()) {
    rcv_queue.EnableInterrupts();
    // Double check to avoid race
    if (likely(!rcv_queue.HasUsedBuffer())) {
      receive_callback_.Stop();
      return;
    } else {
      // raced, disable interrupts
      rcv_queue.DisableInterrupts();
      goto process;
    }
  }

  kassert(circ_buffer_head_ != circ_buffer_tail_);
  kassert(circ_buffer_[circ_buffer_tail_ % 256]);
  auto b = std::move(circ_buffer_[circ_buffer_tail_ % 256]);
  ++circ_buffer_tail_;
  // kassert(rcv_queue.HasUsedBuffer());
  // auto b = rcv_queue.GetBuffer();
  auto dp = b->GetDataPointer();
  auto& vheader = dp.Get<VirtioNetHeader>();
  auto good_csum = true;
  if (!guest_csum_) {
    good_csum = rx_csum(b);
  } else if (vheader.flags & VirtioNetHeader::kNeedsCsum) {
    kabort("handle partial checksum of packet\n");
  }

  if (rcv_queue.num_free_descriptors() * 2 >= rcv_queue.Size()) {
    FillRxRing();
  }

  if (good_csum) {
    kassert(b->CountChainElements() == 1);
    kassert(b->Unique());
    b->Advance(sizeof(VirtioNetHeader));
    itf_->Receive(std::move(b));
  } else {
    kprintf("drop\n");
  }
}
