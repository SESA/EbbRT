//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NETETH_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NETETH_H_

namespace ebbrt {
const constexpr size_t kEthHwAddrLen = 6;

typedef std::array<uint8_t, kEthHwAddrLen> EthernetAddress;

struct __attribute__((packed)) EthernetHeader {
  EthernetAddress dst;
  EthernetAddress src;
  uint16_t type;
};

const constexpr uint16_t kEthTypeIp = 0x800;
const constexpr uint16_t kEthTypeArp = 0x806;
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NETETH_H_
