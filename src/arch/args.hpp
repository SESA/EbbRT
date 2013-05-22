#ifndef EBBRT_ARCH_ARGS_HPP
#define EBBRT_ARCH_ARGS_HPP

#ifdef ARCH_X86_64
#include "arch/x86_64/args.hpp"
#else
#error "Unsupported Architecture"
#endif
#endif
