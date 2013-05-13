#ifndef EBBRT_LRT_TRANS_IMPL_HPP
#error "Don't include this file directly"
#endif

#ifdef ARCH_X86_64
#include "lrt/arch/x86_64/trans_impl.hpp"
#else
#error "Unsupported Architecture"
#endif
