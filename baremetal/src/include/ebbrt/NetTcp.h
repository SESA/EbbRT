//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NETTCP_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NETTCP_H_

namespace ebbrt {
const constexpr size_t kTcpMss = 1460;
const constexpr uint32_t kTcpWnd = 1 << 21;
const constexpr uint8_t kWindowShift = 7;

const constexpr uint16_t TcpWindow16(uint32_t sz) {
  return sz >> kWindowShift;
}

const constexpr uint16_t kTcpFin = 0x01;
const constexpr uint16_t kTcpSyn = 0x02;
const constexpr uint16_t kTcpRst = 0x04;
const constexpr uint16_t kTcpPsh = 0x08;
const constexpr uint16_t kTcpAck = 0x10;
const constexpr uint16_t kTcpUrg = 0x20;
const constexpr uint16_t kTcpEce = 0x40;
const constexpr uint16_t kTcpCwr = 0x80;

const constexpr uint16_t kTcpFlagMask = 0x3f;

struct __attribute__((packed)) TcpHeader {
  void SetHdrLenFlags(size_t header_len, uint16_t flags) {
    auto header_words = header_len / 4;
    hdrlen_flags = htons((header_words << 12) | flags);
  }

  void SetFlags(uint16_t flags) {
    hdrlen_flags &= ~htons(0xFFF);
    hdrlen_flags |= htons(flags & 0xFFF);
  }

  size_t HdrLen() const { return (ntohs(hdrlen_flags) >> 12) * 4; }

  uint16_t Flags() const { return ntohs(hdrlen_flags) & kTcpFlagMask; }
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seqno;
  uint32_t ackno;
  uint16_t hdrlen_flags;
  uint16_t wnd;
  uint16_t checksum;
  uint16_t urgp;
  char options[];
};

struct TcpInfo {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seqno;
  uint32_t ackno;
  size_t tcplen;
};

}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NETTCP_H_
