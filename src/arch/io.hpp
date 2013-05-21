#ifndef EBBRT_ARCH_IO_HPP
#define EBBRT_ARCH_IO_HPP

#include <cstdint>

namespace ebbrt {
  inline void out8(uint8_t val, uint16_t port);
  inline uint8_t in8(uint16_t port);
  inline void out16(uint16_t val, uint16_t port);
  inline uint16_t in16(uint16_t port);
  inline void out32(uint32_t val, uint16_t port);
  inline uint32_t in32(uint16_t port);
}

#ifdef ARCH_X86_64
#include "arch/x86_64/io.hpp"
#else
#error "Unsupported Architecture"
#endif

#endif
