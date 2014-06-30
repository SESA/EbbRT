//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NETICMP_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NETICMP_H_

namespace ebbrt {
struct __attribute__((packed)) IcmpHeader {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint32_t remainder;
};

const constexpr uint8_t kIcmpEchoReply = 0;
const constexpr uint8_t kIcmpEchoRequest = 8;
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NETICMP_H_
