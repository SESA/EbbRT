//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NETCHECKSUM_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NETCHECKSUM_H_

#include <cstdint>
#include <cstring>

#include <ebbrt/IOBuf.h>
#include <ebbrt/NetIp.h>

namespace ebbrt {
uint16_t OffloadPseudoCsum(const IOBuf& buf, uint8_t proto, Ipv4Address src,
                           Ipv4Address dst);
uint16_t IpPseudoCsum(const IOBuf& buf, uint8_t proto, Ipv4Address src,
                      Ipv4Address dst);
uint16_t IpCsum(const IOBuf& buf);
uint16_t IpCsum(const uint8_t* buf, size_t len);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NETCHECKSUM_H_
