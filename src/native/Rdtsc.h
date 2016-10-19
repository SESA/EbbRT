//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_RDTSC_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_RDTSC_H_

namespace ebbrt {
inline uint64_t rdtsc() {
  uint64_t tsc;
  asm volatile("rdtsc;"
               "shl $32,%%rdx;"
               "or %%rdx,%%rax"
               : "=a"(tsc)
               :
               : "%rcx", "%rdx");
  return tsc;
}

inline uint64_t rdtscp() {
  uint64_t tsc;
  asm volatile("rdtscp;"
               "shl $32,%%rdx;"
               "or %%rdx,%%rax"
               : "=a"(tsc)
               :
               : "%rcx", "%rdx");
  return tsc;
}
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_RDTSC_H_
