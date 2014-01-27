//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_RDTSC_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_RDTSC_H_

namespace ebbrt {
inline uint64_t rdtsc() {
  uint32_t hi, lo;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}
}

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_RDTSC_H_
