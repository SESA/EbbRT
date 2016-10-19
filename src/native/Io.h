//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_IO_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_IO_H_

#include <cstdint>

namespace ebbrt {
namespace io {
inline void Out8(uint16_t port, uint8_t val) noexcept {
  asm volatile("outb %b[val], %w[port]"
               :  // no output
               : [val] "a"(val), [port] "Nd"(port));
}
inline void Out16(uint16_t port, uint16_t val) noexcept {
  asm volatile("outw %w[val], %w[port]"
               :  // no output
               : [val] "a"(val), [port] "Nd"(port));
}
inline void Out32(uint16_t port, uint32_t val) noexcept {
  asm volatile("outl %[val], %w[port]"
               :  // no output
               : [val] "a"(val), [port] "Nd"(port));
}

inline uint8_t In8(uint16_t port) noexcept {
  uint8_t val;
  asm volatile("inb %w[port], %b[val]" : [val] "=a"(val) : [port] "Nd"(port));
  return val;
}

inline uint16_t In16(uint16_t port) noexcept {
  uint16_t val;
  asm volatile("inw %w[port], %w[val]" : [val] "=a"(val) : [port] "Nd"(port));
  return val;
}

inline uint32_t In32(uint16_t port) noexcept {
  uint32_t val;
  asm volatile("inl %w[port], %[val]" : [val] "=a"(val) : [port] "Nd"(port));
  return val;
}
}  // namespace io
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_IO_H_
