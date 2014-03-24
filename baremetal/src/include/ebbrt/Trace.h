//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_TRACE_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_TRACE_H_

#include <cstdint>
#include <string>

namespace ebbrt {
namespace trace {
__attribute__((no_instrument_function)) void Init() ;
__attribute__((no_instrument_function)) void Dump() ;
__attribute__((no_instrument_function)) void Enable();
__attribute__((no_instrument_function)) void Disable();
__attribute__((no_instrument_function)) void AddTracepoint(std::string);
__attribute__((no_instrument_function)) bool IsIntel();

inline uint64_t rdtsc(void) {
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}
inline uint64_t rdpmc(int reg) {
  uint32_t lo, hi;
  __asm__ __volatile__("rdpmc" : "=a"(lo), "=d"(hi) : "c"(reg));
  return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}
inline void cpuid(uint32_t eax, uint32_t ecx, uint32_t* a, uint32_t* b,
                  uint32_t* c, uint32_t* d) {
  __asm__ __volatile__("cpuid"
                       : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
                       : "a"(eax), "c"(ecx));
}

typedef struct {
  union {
    uint64_t val;
    struct {
      uint64_t eventselect_7_0 : 8;
      uint64_t unitmask : 8;
      uint64_t usermode : 1;
      uint64_t osmode : 1;
      uint64_t edge : 1;
      uint64_t pin : 1;
      uint64_t enint : 1;
      uint64_t reserved2 : 1;
      uint64_t en : 1;
      uint64_t inv : 1;
      uint64_t cntmask : 8;
      uint64_t reserved0 : 32;
    } __attribute__((packed));
  };
} perf_event_select;

typedef struct {
  union {
    uint64_t val;
    struct {
      uint64_t ctr0_enable : 2;
      uint64_t ctr0_reserved : 1;
      uint64_t ctr0_pmi : 1;
      uint64_t ctr1_enable : 2;
      uint64_t ctr1_reserved : 1;
      uint64_t ctr1_pmi : 1;
      uint64_t ctr2_enable : 2;
      uint64_t ctr2_reserved : 1;
      uint64_t ctr2_pmi : 1;
      uint64_t reserved : 52;
    } __attribute__((packed));
  };
} fixed_ctr_ctrl;

typedef struct {
  union {
    uint64_t val;
    struct {
      uint64_t pci0_enable : 1;
      uint64_t pci1_enable : 1;
      uint64_t reserved0 : 30;
      uint64_t ctr0_enable : 1;
      uint64_t ctr1_enable : 1;
      uint64_t ctr2_enable : 1;
      uint64_t reserved1 : 29;
    } __attribute__((packed));
  };
} perf_global_ctrl;

typedef struct {
  uint8_t status;
  union {
    char point[40];
    struct {
      uintptr_t func;
      uintptr_t caller;
      uint64_t time;
      uint64_t cycles;
      uint64_t instructions;
    } __attribute__((packed));
  };
} trace_entry;

}
}
#endif 
