//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NETMISC_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NETMISC_H_

namespace ebbrt {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
constexpr uint16_t htons(uint16_t data) {
  return ((data & 0xff00) >> 8) | ((data & 0xff) << 8);
}
#else
constexpr uint16_t htons(uint16_t data) { return data; }
#endif
constexpr uint16_t ntohs(uint16_t data) { return htons(data); }
}

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NETMISC_H_
