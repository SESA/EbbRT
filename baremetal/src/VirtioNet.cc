//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/VirtioNet.h>

#include "lwip/ip.h"
#include "lwip/inet_chksum.h"
#include "lwip/tcp_impl.h"
#include "lwip/udp.h"

#include <ebbrt/Debug.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/UniqueIOBuf.h>

namespace {
const constexpr int kCSum = 0;
const constexpr int kGuestCSum = 1;
const constexpr int kMac = 5;
const constexpr int kMrgRxbuf = 15;
const constexpr int kNotifyOnEmpty = 24;
}

ebbrt::VirtioNetDriver::VirtioNetDriver(pci::Device& dev)
    : VirtioDriver<VirtioNetDriver>(dev),
      allocator_(SlabAllocator::Construct(2048)),
      itf_(network_manager->NewInterface(*this)),
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
}

void ebbrt::VirtioNetDriver::FillRxRing() {
  auto& rcv_queue = GetQueue(0);
  auto num_bufs = rcv_queue.num_free_descriptors();
  auto bufs = std::vector<std::unique_ptr<MutIOBuf>>();
  bufs.reserve(num_bufs);

  for (size_t i = 0; i < num_bufs; ++i) {
    bufs.emplace_back(MakeUniqueIOBuf(2048));
  }

  auto it = rcv_queue.AddWritableBuffers(bufs.begin(), bufs.end());
  kassert(it == bufs.end());
}

void ebbrt::VirtioNetDriver::FreeSentPackets() {
  auto& send_queue = GetQueue(1);
  send_queue.ClearUsedBuffers();
}

uint32_t ebbrt::VirtioNetDriver::GetDriverFeatures() {
  // return 1 << kCSum | 1 << kGuestCSum | 1 << kMac | 1 << kMrgRxbuf;
  return 1 << kMac | 1 << kMrgRxbuf;
}

// namespace {
// struct ethernet_header {
//   uint8_t dest_addr[6];
//   uint8_t source_addr[6];
//   uint16_t ethertype;
// };

// const constexpr uint16_t kEtherTypeIP = 0x0800;
// const constexpr uint16_t kEtherTypeVLan = 0x8100;

// u16_t lwip_standard_chksum(const void* dataptr, int len) {
//   const uint8_t* pb = reinterpret_cast<const uint8_t*>(dataptr);
//   uint16_t t = 0;
//   u32_t sum = 0;
//   int odd = ((mem_ptr_t)pb & 1);

//   /* Get aligned to u16_t */
//   if (odd && len > 0) {
//     (reinterpret_cast<uint8_t*>(&t))[1] = *pb++;
//     len--;
//   }

//   /* Add the bulk of the data */
//   auto ps = reinterpret_cast<const uint16_t*>(pb);
//   while (len > 1) {
//     sum += *ps++;
//     len -= 2;
//   }

//   /* Consume left-over byte, if any */
//   if (len > 0) {
//     (reinterpret_cast<uint8_t*>(&t))[0] = *reinterpret_cast<const
// uint8_t*>(ps);
//   }

//   /* Add end bytes */
//   sum += t;

//   /* Fold 32-bit sum to 16 bits
//      calling this twice is propably faster than if statements... */
//   sum = FOLD_U32T(sum);
//   sum = FOLD_U32T(sum);

//   /* Swap if alignment was odd */
//   if (odd) {
//     sum = SWAP_BYTES_IN_WORD(sum);
//   }

//   return (u16_t)sum;
// }

// uint16_t inet_chksum_pseudo(const ebbrt::IOBuf& buf,
//                             ip_addr_t* src, ip_addr_t* dest, uint8_t proto,
//                             uint16_t proto_len, uint16_t offset = 0) {
//   uint32_t acc = 0;
//   uint8_t swapped = 0;
//   proto_len -= offset;

//   for (auto& b : buf) {
//     if (b.Length() <= offset) {
//       offset -= b.Length();
//       continue;
//     }
//     acc += lwip_standard_chksum(b.Data() + offset, b.Length() - offset);
//     offset = 0;
//     acc = FOLD_U32T(acc);
//     if (b.Length() % 2 != 0) {
//       swapped = 1 - swapped;
//       acc = SWAP_BYTES_IN_WORD(acc);
//     }
//   }

//   if (swapped) {
//     acc = SWAP_BYTES_IN_WORD(acc);
//   }
//   uint32_t addr = ip4_addr_get_u32(src);
//   acc += (addr & 0xffffUL);
//   acc += ((addr >> 16) & 0xffffUL);
//   addr = ip4_addr_get_u32(dest);
//   acc += (addr & 0xffffUL);
//   acc += ((addr >> 16) & 0xffffUL);
//   acc += (u32_t)htons((u16_t)proto);
//   acc += (u32_t)htons(proto_len);

//   /* Fold 32-bit sum to 16 bits
//      calling this twice is propably faster than if statements... */
//   acc = FOLD_U32T(acc);
//   acc = FOLD_U32T(acc);
//   return (uint16_t) ~(acc & 0xffffUL);
// }

// void tx_csum(const ebbrt::IOBuf& buf) {
//   auto dp = buf.GetDataPointer();
//   auto& eh = dp.Get<ethernet_header>();
//   auto eth_type = ntohs(eh.ethertype);
//   ebbrt::kbugon(eth_type == kEtherTypeVLan, "VLAN csum not supported\n");
//   if (eth_type != kEtherTypeIP) {
//     return;
//   }

//   auto& ih = const_cast<ip_hdr&>(dp.GetNoAdvance<ip_hdr>());
//   auto iphdr_hlen = IPH_HL(&ih) * 4;
//   auto offset = sizeof(ethernet_header) + iphdr_hlen;
//   ip_addr_t src;
//   ip_addr_copy(src, ih.src);
//   ip_addr_t dest;
//   ip_addr_copy(dest, ih.dest);
//   dp.Advance(iphdr_hlen);
//   switch (IPH_PROTO(&ih)) {
//   case IP_PROTO_UDP: {
//     auto& uh = const_cast<udp_hdr&>(dp.Get<udp_hdr>());
//     uh.chksum = 0;
//     auto chksum = inet_chksum_pseudo(buf, &src, &dest, IP_PROTO_UDP,
//                                      buf.ComputeChainDataLength(), offset);
//     if (chksum == 0x0000)
//       chksum = 0xffff;
//     uh.chksum = chksum;
//     break;
//   }
//   case IP_PROTO_TCP: {
//     auto& th = const_cast<tcp_hdr&>(dp.Get<tcp_hdr>());
//     th.chksum = 0;
//     th.chksum = inet_chksum_pseudo(buf, &src, &dest, IP_PROTO_TCP,
//                                    buf.ComputeChainDataLength(), offset);
//     break;
//   }
//   }
// }

// bool rx_csum(ebbrt::MutIOBuf& buf) {
//   auto dp = buf.GetDataPointer();
//   auto& eh = dp.Get<ethernet_header>();
//   auto eth_type = ntohs(eh.ethertype);
//   ebbrt::kbugon(eth_type == kEtherTypeVLan, "VLAN csum not supported\n");
//   if (eth_type != kEtherTypeIP) {
//     return true;
//   }

//   auto& ih = dp.GetNoAdvance<ip_hdr>();
//   auto iphdr_hlen = IPH_HL(&ih) * 4;
//   if (inet_chksum(const_cast<void*>(static_cast<const void*>(&ih)),
//                   iphdr_hlen) != 0) {
//     ebbrt::kprintf("ip chksum fail\n");
//     return false;
//   }

//   kassert(buf.CountChainElements() == 1);
//   buf.Advance(sizeof(ethernet_header) + iphdr_hlen);
//   ip_addr_t src;
//   ip_addr_copy(src, ih.src);
//   ip_addr_t dest;
//   ip_addr_copy(dest, ih.dest);
//   bool ret = true;
//   switch (IPH_PROTO(&ih)) {
//   case IP_PROTO_UDP: {
//     // handle only single element chain for now
//     if (inet_chksum_pseudo(buf, &src, &dest, IP_PROTO_UDP,
//                            buf.ComputeChainDataLength()) != 0) {
//       ebbrt::kprintf("udp chksum fail\n");
//       ret = false;
//     }
//     break;
//   }
//   case IP_PROTO_TCP: {
//     if (inet_chksum_pseudo(buf, &src, &dest, IP_PROTO_TCP,
//                            buf.ComputeChainDataLength()) != 0) {
//       ebbrt::kprintf("tcp chksum fail\n");
//       ret = false;
//     }
//     break;
//   }
//   }
//   buf.Retreat(sizeof(ethernet_header) + iphdr_hlen);
//   return ret;
// }
// }  // namespace

void ebbrt::VirtioNetDriver::Send(std::unique_ptr<IOBuf> buf) {
  std::unique_ptr<MutUniqueIOBuf> b;
  std::lock_guard<ebbrt::SpinLock> lock(send_mutex_);

  auto& send_queue = GetQueue(1);
  send_queue.ClearUsedBuffers();
  auto free_desc = send_queue.num_free_descriptors();
  // if (free_desc > buf->CountChainElements()) {
  //   // we have enough descriptors to avoid a copy
  //   b = MakeUniqueIOBuf(sizeof(VirtioNetHeader), /* zero_memory = */ true);
  //   b->PrependChain(std::move(buf));
  // } else
  if (free_desc >= 1) {
    // XXX: Maybe we should use indirect descriptors instead?
    // copy into one buffer
    auto len = buf->ComputeChainDataLength();
    b = MakeUniqueIOBuf(len + sizeof(VirtioNetHeader));
    memset(b->MutData(), 0, sizeof(VirtioNetHeader));
    auto data = b->MutData() + sizeof(VirtioNetHeader);
    for (auto& buf_it : *buf) {
      memcpy(data, buf_it.Data(), buf_it.Length());
      data += buf_it.Length();
    }
  } else {
    kprintf("Drop\n");
    // kick to make it process more buffers, drop the send
    send_queue.Kick();
    return;
  }

  send_queue.AddReadableBuffer(std::move(b));
}

const ebbrt::EthernetAddress& ebbrt::VirtioNetDriver::GetMacAddress() {
  return mac_addr_;
}

void ebbrt::VirtioNetDriver::Poll() { FreeSentPackets(); }

void ebbrt::VirtioNetDriver::ReceivePoll() {
  auto& rcv_queue = GetQueue(0);
process:
  rcv_queue.ProcessUsedBuffers([this](std::unique_ptr<MutIOBuf> buf) {
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
  // auto good_csum = true;
  // if (!guest_csum_) {
  //   good_csum = rx_csum(*b);
  // }

  if (rcv_queue.num_free_descriptors() * 2 >= rcv_queue.Size()) {
    FillRxRing();
  }

  // if (good_csum) {
  kassert(b->CountChainElements() == 1);
  b->Advance(sizeof(VirtioNetHeader));
  itf_.Receive(std::move(b));
  // } else {
  //   kprintf("drop\n");
  // }
}
