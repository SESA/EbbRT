//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_TRACE_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_TRACE_H_

#include <cstdint>

namespace ebbrt {
namespace trace {
__attribute__((no_instrument_function)) void Init() ;
__attribute__((no_instrument_function)) void Dump() ;
__attribute__((no_instrument_function)) void enable_trace();
__attribute__((no_instrument_function)) void disable_trace();


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
  bool enter;
  uintptr_t func;
  uintptr_t caller;
  uint64_t time;
  uint64_t cycles;
  uint64_t instructions;
} trace_entry;



}
}
#endif 
