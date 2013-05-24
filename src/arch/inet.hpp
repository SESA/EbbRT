#ifndef EBBRT_ARCH_INET_HPP
#define EBBRT_ARCH_INET_HPP

namespace ebbrt {
  inline uint16_t htons(uint16_t hostshort);
}

#ifdef ARCH_X86_64
#include "arch/x86_64/inet.hpp"
#else
#error "Unsupported Architecture"
#endif

#endif
