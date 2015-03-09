//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_VIRTIONET_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_VIRTIONET_H_

#include <ebbrt/MulticoreEbb.h>
#include <ebbrt/Net.h>
#include <ebbrt/SlabAllocator.h>
#include <ebbrt/SpinLock.h>
#include <ebbrt/Virtio.h>

namespace ebbrt {
class VirtioNetRep;

class VirtioNetDriver : public VirtioDriver<VirtioNetDriver>,
                        public EthernetDevice {
 public:
  static const constexpr uint16_t kDeviceId = 0x1000;

  explicit VirtioNetDriver(pci::Device& dev);

  static void Create(pci::Device& dev);
  static uint32_t GetDriverFeatures();
  void Send(std::unique_ptr<IOBuf> buf, PacketInfo pinfo) override;
  const EthernetAddress& GetMacAddress() override;

 private:
  void FillRxRing();
  void FreeSentPackets();
  void ReceivePoll();

  EbbRef<VirtioNetRep> ebb_;
  EthernetAddress mac_addr_;
  NetworkManager::Interface& itf_;
  VRing* ctrl_queue_;

  friend class VirtioNetRep;
};

class VirtioNetRep : public MulticoreEbb<VirtioNetRep, VirtioNetDriver> {
 public:
  explicit VirtioNetRep(const VirtioNetDriver& root);
  void Send(std::unique_ptr<IOBuf> buf, PacketInfo pinfo);
  void Receive();

 private:
  void FillRxRing();
  void ReceivePoll();

  struct VirtioNetHeader {
    static const constexpr uint8_t kNeedsCsum = 1;
    static const constexpr uint8_t kGsoNone = 0;
    static const constexpr uint8_t kGsoTcpv4 = 1;
    static const constexpr uint8_t kGsoUdp = 3;
    static const constexpr uint8_t kGsoTcpv6 = 4;
    static const constexpr uint8_t kGsoEvn = 0x80;

    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    uint16_t num_buffers;
  };

  const VirtioNetDriver& root_;
  VirtioDriver<VirtioNetDriver>::VRing& rcv_queue_;
  VirtioDriver<VirtioNetDriver>::VRing& snd_queue_;
  EventManager::IdleCallback receive_callback_;
  size_t circ_buffer_head_;
  size_t circ_buffer_tail_;
  std::array<std::unique_ptr<MutIOBuf>, 256> circ_buffer_;
};
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_VIRTIONET_H_
