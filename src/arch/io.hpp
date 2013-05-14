#ifndef EBBRT_ARCH_IO_HPP
#define EBBRT_ARCH_IO_HPP

#include <cstdint>

namespace ebbrt {
    inline void outb(int8_t val, int16_t port);
    inline int8_t inb(int16_t port);
}

#ifdef ARCH_X86_64
#include "arch/x86_64/io.hpp"
#else
#error "Unsupported Architecture"
#endif

#endif
