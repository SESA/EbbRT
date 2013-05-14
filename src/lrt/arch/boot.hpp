#ifndef EBBRT_LRT_BOOT_HPP
#error "Don't include this file directly"
#endif

#ifdef ARCH_X86_64
#include "lrt/arch/x86_64/boot.hpp"
#else
#error "Unsupported architecture"
#endif
