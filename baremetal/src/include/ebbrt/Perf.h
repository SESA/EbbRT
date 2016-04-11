//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_PERF_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_PERF_H_

namespace ebbrt {
namespace perf {
#define PERF_CPUID_LEAF 0x0A
#define FIXED_CTL_RING_LVL_NONE 0x0
#define FIXED_CTL_RING_LVL_OS 0x1
#define FIXED_CTL_RING_LVL_USR 0x2
#define FIXED_CTL_RING_LVL_ALL 0x3
#define IA32_FIXED_CTR_CTRL_MSR 0x38D
#define IA32_PERF_GLOBAL_CTRL_MSR 0x38F
#define IA32_PERF_GLOBAL_STATUS_MSR 0x38E
#define IA32_PERF_GLOBAL_OVF_CTRL_MSR 0x390
#define IA32_FXD_PMC(X) (0x309 + (X))
#define PERF_EVENT_BITMASK(X) (1 << X)
#define IA32_PERFEVTSEL_MSR(X) (0x186 + (X))
#define IA32_PMC(X) (0x0C1 + (X))
#define IA32_A_PMC(X) (0x4C1 + (X))
#define FIXED_EVT_OFFSET(X) (0x20 + (X))
#define NIL_EVT_OFFSET 0xFF
// cycles
#define PERFEVTSEL_EVT_CYCLES 0x3C
#define PERFEVTSEL_UMASK_CYCLES 0x00
// reference cycles
#define PERFEVTSEL_EVT_CYCLES_REF 0x3C
#define PERFEVTSEL_UMASK_CYCLES_REF 0x01
// instructions
#define PERFEVTSEL_EVT_INSTRUCTIONS 0xC0
#define PERFEVTSEL_UMASK_INSTRUCTIONS 0x00
// last-level cache references
#define PERFEVTSEL_EVT_LLC_REF 0x2E
#define PERFEVTSEL_UMASK_LLC_REF 0x4F
// last-level cache misses
#define PERFEVTSEL_EVT_LLC_MISSES 0x2E
#define PERFEVTSEL_UMASK_LLC_MISSES 0x41
// branch instructions
#define PERFEVTSEL_EVT_BRANCH_INSTRUCTIONS 0xC4
#define PERFEVTSEL_UMASK_BRANCH_INSTRUCTIONS 0x00
// branch misses
#define PERFEVTSEL_EVT_BRANCH_MISSES 0xC5
#define PERFEVTSEL_UMASK_BRANCH_MISSES 0x00

enum class PerfEvent : uint8_t {
  cycles = 0x0,
  instructions,
  reference_cycles,
  llc_references,
  llc_misses,
  branch_instructions,
  branch_misses,
  fixed_instructions = FIXED_EVT_OFFSET(0),
  fixed_cycles,
  fixed_reference_cycles,
  /* add architecture-specific events here */
  nil_event = NIL_EVT_OFFSET
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
} ia32_perfevtsel_t;

typedef struct {
  union {
    uint64_t val;
    struct {
      uint64_t pci0 : 1;
      uint64_t pci1 : 1;
      uint64_t pci2 : 1;
      uint64_t pci3 : 1;
      uint64_t pci4 : 1;
      uint64_t pci5 : 1;
      uint64_t pci6 : 1;
      uint64_t pci7 : 1;
      uint64_t reserved0 : 24;
      uint64_t ctr0 : 1;
      uint64_t ctr1 : 1;
      uint64_t ctr2 : 1;
      uint64_t reserved1 : 27;
      uint64_t ovfbuf : 1;
      uint64_t conchd : 1;
    } __attribute__((packed));
  };
} ia32_perf_global_t;

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
} ia32_fixed_ctr_ctrl_t;

class PerfCounter {
 public:
  PerfCounter();
  PerfCounter(PerfEvent e);
  ~PerfCounter();
  void Clear();
  bool Overflow();
  uint64_t Read();
  void Start();
  void Stop();
  int8_t AvailablePMCs();

 private:
  PerfEvent evt_;
  uint8_t evt_num_;
  uint8_t pmc_num_;
  uint8_t pmc_version_;
  uint8_t pmc_count_;
  uint8_t pmc_width_;
  uint8_t pmc_events_;
  uint8_t pmc_fixed_count_;
  uint8_t pmc_fixed_width_;
  uint64_t counter_offset_;
};

}  // namespace perf
}  // namespace ebbrt
#endif
