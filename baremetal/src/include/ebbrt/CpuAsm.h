//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_CPUASM_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_CPUASM_H_

namespace ebbrt {
inline uintptr_t ReadCr3() {
  uintptr_t cr3;
  asm volatile("mov %%cr3, %[cr3]" : [cr3] "=r"(cr3));
  return cr3;
}
}

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_CPUASM_H_
