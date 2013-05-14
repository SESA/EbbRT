#ifndef EBBRT_ARCH_CPU_HPP
#define EBBRT_ARCH_CPU_HPP

#ifdef ARCH_X86_64
#include "arch/x86_64/cpu.hpp"
#else
#error "Unsupported Architecture"
#endif

#endif
