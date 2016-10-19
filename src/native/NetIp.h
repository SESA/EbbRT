//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NETIP_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NETIP_H_

#include "NetIpAddress.h"
#include "NetMisc.h"

namespace ebbrt {
struct __attribute__((packed)) Ipv4Header {
  static const constexpr uint16_t kReservedFlag = 0x8000;
  static const constexpr uint16_t kDontFragment = 0x4000;
  static const constexpr uint16_t kMoreFragments = 0x2000;
  static const constexpr uint16_t kFragOffMask = 0x1fff;

  uint8_t Version() const { return version_ihl >> 4; }

  uint8_t HeaderLength() const {
    auto words = version_ihl & 0xf;
    return words * 4;
  }

  uint16_t TotalLength() const { return ntohs(length); }

  bool Fragmented() const { return MoreFragments() || FragmentOffset(); }

  bool MoreFragments() const { return ntohs(flags_fragoff) & kMoreFragments; }

  uint16_t FragmentOffset() const {
    return ntohs(flags_fragoff) & kFragOffMask;
  }

  uint16_t ComputeChecksum() const {
    auto hptr = this;
    uint32_t header_words = version_ihl & 0xf;
    uint32_t sum;
    asm("movl (%[hptr]), %[sum];"  // grabs bytes 0-3
        "subl $4, %[header_words];"  // header words should be > 1 now
        "addl 4(%[hptr]), %[sum];"  // add 4-7
        "adcl 8(%[hptr]), %[sum];"  // add 8-11
        "adcl 12(%[hptr]), %[sum];"  // add 12-15
        "1: adcl 16(%[hptr]), %[sum];"  // add 16-19
        "lea 4(%[hptr]), %[hptr];"  // advance ptr by 4 so we can reuse the
        // above instruction if there are more words
        "decl %[header_words];"
        "jne 1b;"  // keep adding if there are options
        "adcl $0, %[sum];"  // add the carry
        "movl %[sum], %[header_words];"  // temp store in header_words
        "shrl $16, %[sum];"  // sum = upper 16 bits
        "addw %w[header_words], %w[sum];"  // sum lower 16 bits with upper 16
        "adcl $0, %[sum];"  // add carry
        "notl %[sum];"  // take the ones complement
        : [hptr] "+r"(hptr), [sum] "=r"(sum), [header_words] "+r"(header_words)
        : "g"(*hptr));

    return sum;
  }

  uint8_t version_ihl;
  uint8_t dscp_ecn;
  uint16_t length;
  uint16_t id;
  uint16_t flags_fragoff;
  uint8_t ttl;
  uint8_t proto;
  uint16_t chksum;
  Ipv4Address src;
  Ipv4Address dst;
};

const constexpr uint8_t kIpDefaultTtl = 255;

const constexpr uint16_t kIpProtoICMP = 1;
const constexpr uint16_t kIpProtoTCP = 6;
const constexpr uint16_t kIpProtoUDP = 17;
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NETIP_H_
