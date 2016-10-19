//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_PERF_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_PERF_H_
#include "Msr.h"

namespace ebbrt {
namespace perf {
/*
 * Performance-monitoring facilities & events can be found in the
 * Intel Programmer's Manual Vol. 3B, Chapters 18 & 19
 * */
const constexpr uint8_t kIa32FixedCtlRingLvlAll = 0x3;
const constexpr uint8_t kIa32FixedCtlRingLvlNone = 0x0;
const constexpr uint16_t kIa32FixedCtrCtrlMsr = 0x38D;
const constexpr uint16_t kIa32PerfGlobalCtrlMsr = 0x38F;
const constexpr uint16_t kIa32PerfGlobalOvlCtrlMsr = 0x390;
const constexpr uint16_t kIa32PerfGlobalStatusMsr = 0x38E;
const constexpr uint8_t kNilEvtOffset = 0xFF;
const constexpr uint8_t kPerfCpuLeaf = 0x0A;
const constexpr uint8_t kPerfEvtSelEvtBranchInstructions = 0xC4;
const constexpr uint8_t kPerfEvtSelEvtBranchMisses = 0xC5;
const constexpr uint8_t kPerfEvtSelEvtCycles = 0x3C;
const constexpr uint8_t kPerfEvtSelEvtCyclesRef = 0x3C;
const constexpr uint8_t kPerfEvtSelEvtInstructions = 0xC0;
const constexpr uint8_t kPerfEvtSelEvtLLCMisses = 0x2E;
const constexpr uint8_t kPerfEvtSelEvtLLCRef = 0x2E;
const constexpr uint8_t kPerfEvtSelEvtTlbLoadMisses = 0x08;
const constexpr uint8_t kPerfEvtSelEvtTlbStoreMisses = 0x49;
const constexpr uint8_t kPerfEvtSelUmaskBranchInstructions = 0x00;
const constexpr uint8_t kPerfEvtSelUmaskBranchMisses = 0x00;
const constexpr uint8_t kPerfEvtSelUmaskCycles = 0x00;
const constexpr uint8_t kPerfEvtSelUmaskCyclesRef = 0x01;
const constexpr uint8_t kPerfEvtSelUmaskInstructions = 0x00;
const constexpr uint8_t kPerfEvtSelUmaskLLCMisses = 0x41;
const constexpr uint8_t kPerfEvtSelUmaskLLCRef = 0x4F;
const constexpr uint8_t kPerfEvtSelUmaskTlbLoadMisses = 0x01;
const constexpr uint8_t kPerfEvtSelUmaskTlbStoreMisses = 0x0E;
constexpr uint8_t kFixedEvtOffset(uint8_t x) { return 0x20 + x; }
constexpr uint16_t kIa32FxdPmc(uint8_t x) { return 0x309 + x; }
constexpr uint16_t kIa32PerfEvtSelMsr(uint8_t x) { return 0x186 + x; }
constexpr uint8_t kIa32Pmc(uint8_t x) { return 0xC1 + x; }

extern thread_local uint64_t perf_global_ctrl;

enum class PerfEvent : uint8_t {
  cycles = 0x0,
  instructions,
  reference_cycles,
  llc_references,
  llc_misses,
  branch_instructions,
  branch_misses,
  fixed_instructions = kFixedEvtOffset(0),
  fixed_cycles,
  fixed_reference_cycles,
  /* add architecture-specific events here */
  tlb_load_misses,
  tlb_store_misses,
  /**/
  nil_event = kNilEvtOffset
};

typedef struct {
  union {
    uint64_t val;
    struct {
      uint64_t event : 8;
      uint64_t umask : 8;
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
} PerfEvtSelMsr;
/* Structure for Intel IA32_PERFEVTSELx MSRs (see 18.2 in the Intel
 * Programmer's Manual Vol. 3B) */

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
} FixedCtrCtrl;
/* Structure for Intel IA32_FIXED_CTR_CTRL MSR (see 18.2 in the Intel
 * Programmer's Manual Vol. 3B) */

class PerfCounter {
 public:
  PerfCounter() : evt_{PerfEvent::nil_event} {};
  explicit PerfCounter(PerfEvent e);
  PerfCounter(PerfCounter&& other) {
    evt_ = other.evt_;
    pmc_num_ = other.pmc_num_;
    counter_offset_ = other.counter_offset_;
    other.evt_ = ebbrt::perf::PerfEvent::nil_event;
  }
  PerfCounter& operator=(PerfCounter&& other) {
    evt_ = other.evt_;
    pmc_num_ = other.pmc_num_;
    counter_offset_ = other.counter_offset_;
    other.evt_ = ebbrt::perf::PerfEvent::nil_event;
    return *this;
  }
  PerfCounter(const PerfCounter& other) = delete;
  PerfCounter& operator=(const PerfCounter& other) = delete;
  ~PerfCounter();
  void Clear();
  bool Overflow();
  uint64_t Read();
  void Start() __attribute__((always_inline)) {
    if (evt_ != PerfEvent::nil_event) {
      perf_global_ctrl |= (1ull << pmc_num_);
      ebbrt::msr::Write(kIa32PerfGlobalCtrlMsr, perf_global_ctrl);
    }
    return;
  }
  void Stop() __attribute__((always_inline)) {
    if (evt_ != PerfEvent::nil_event) {
      perf_global_ctrl &= ~(1ull << pmc_num_);
      ebbrt::msr::Write(kIa32PerfGlobalCtrlMsr, perf_global_ctrl);
    }
    return;
  }

 private:
  PerfEvent evt_;
  uint8_t pmc_num_;
  uint64_t counter_offset_;
};
}  // namespace perf
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_PERF_H_
