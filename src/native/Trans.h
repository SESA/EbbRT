//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_TRANS_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_TRANS_H_

#include <cstdint>
#include <cstring>

#include "Idt.h"

namespace ebbrt {
namespace trans {
// last 2^32 bits of virtual address space are reserved for translation tables
const constexpr uintptr_t kVMemStart = 0xFFFFFFFF00000000;

void Init();
void ApInit(size_t index);
void HandleFault(idt::ExceptionFrame* ef, uintptr_t fault_addr);
}
}

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_TRANS_H_
