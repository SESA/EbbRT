//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#include <ebbrt/CpuAsm.h>
#include <ebbrt/Cpuid.h>
#include <ebbrt/Debug.h>
#include <ebbrt/Perf.h>

namespace {
thread_local uint64_t perf_global_ctrl{0};  // stored activated counters
thread_local uint8_t pmcs{0};  // stores allocated gp counters
};

ebbrt::perf::PerfCounter::~PerfCounter() {

  if (evt_ == PerfEvent::nil_event) {
    return;
  }

  // stop counter
  perf_global_ctrl &= ~(1ull << pmc_num_);
  ebbrt::wrmsr(perf_global_ctrl, IA32_PERF_GLOBAL_CTRL_MSR);

  // disable counter
  ia32_fixed_ctr_ctrl_t fixed_ctrl;
  ia32_perfevtsel_t perfevtsel;
  perfevtsel.val = 0;
  fixed_ctrl.val |= ebbrt::rdmsr(IA32_FIXED_CTR_CTRL_MSR);

  switch (evt_) {
  case PerfEvent::nil_event:
    return;
  case PerfEvent::fixed_instructions:
    fixed_ctrl.ctr0_enable = FIXED_CTL_RING_LVL_NONE;
    ebbrt::wrmsr(fixed_ctrl.val, IA32_FIXED_CTR_CTRL_MSR);
    break;
  case PerfEvent::fixed_cycles:
    fixed_ctrl.ctr1_enable = FIXED_CTL_RING_LVL_NONE;
    ebbrt::wrmsr(fixed_ctrl.val, IA32_FIXED_CTR_CTRL_MSR);
    break;
  case PerfEvent::fixed_reference_cycles:
    fixed_ctrl.ctr2_enable = FIXED_CTL_RING_LVL_NONE;
    ebbrt::wrmsr(fixed_ctrl.val, IA32_FIXED_CTR_CTRL_MSR);
    break;
  default:
    perfevtsel.usermode = 0;
    perfevtsel.osmode = 0;
    perfevtsel.en = 0;
    ebbrt::wrmsr(perfevtsel.val, IA32_PERFEVTSEL_MSR(pmc_num_));
    pmcs &= ~(1ull << pmc_num_);
    break;
  };

  return;
}

ebbrt::perf::PerfCounter::PerfCounter(ebbrt::perf::PerfEvent evt) : evt_{evt} {

  auto r = ebbrt::cpuid::Cpuid(PERF_CPUID_LEAF);
  pmc_version_ = r.eax & 0xFF;
  pmc_count_ = (r.eax >> 8) & 0xFF;
  pmc_events_ = (r.ebx) & 0xFF;
  pmc_fixed_count_ = (r.edx) & 0xF;
  pmc_fixed_width_ = (r.edx >> 4) & 0xFF;
  evt_num_ = static_cast<uint8_t>(evt_);

  // check if PMC are supported on this architecture
  if (pmc_version_ == 0) {
    kprintf("Warning: performance monitoring counters are not supported.\n");
    evt_ = PerfEvent::nil_event;
  } else if (((pmc_events_ >> evt_num_) & 0x1) == 1) {
    kprintf("Warning: event type (#%d) is not supported.\n");
    evt_ = PerfEvent::nil_event;
  }

  ia32_fixed_ctr_ctrl_t fixed_ctrl;
  ia32_perfevtsel_t perfevtsel;
  perfevtsel.val = 0;
  fixed_ctrl.val = 0;

  switch (evt_) {
  case PerfEvent::cycles:
    perfevtsel.event = PERFEVTSEL_EVT_CYCLES;
    perfevtsel.umask = PERFEVTSEL_UMASK_CYCLES;
    break;
  case PerfEvent::instructions:
    perfevtsel.event = PERFEVTSEL_EVT_INSTRUCTIONS;
    perfevtsel.umask = PERFEVTSEL_UMASK_INSTRUCTIONS;
    break;
  case PerfEvent::reference_cycles:
    perfevtsel.event = PERFEVTSEL_EVT_CYCLES_REF;
    perfevtsel.umask = PERFEVTSEL_UMASK_CYCLES_REF;
    break;
  case PerfEvent::llc_references:
    perfevtsel.event = PERFEVTSEL_EVT_LLC_REF;
    perfevtsel.umask = PERFEVTSEL_UMASK_LLC_REF;
    break;
  case PerfEvent::llc_misses:
    perfevtsel.event = PERFEVTSEL_EVT_LLC_MISSES;
    perfevtsel.umask = PERFEVTSEL_UMASK_LLC_MISSES;
    break;
  case PerfEvent::branch_instructions:
    perfevtsel.event = PERFEVTSEL_EVT_BRANCH_INSTRUCTIONS;
    perfevtsel.umask = PERFEVTSEL_UMASK_BRANCH_INSTRUCTIONS;
    break;
  case PerfEvent::branch_misses:
    perfevtsel.event = PERFEVTSEL_EVT_BRANCH_MISSES;
    perfevtsel.umask = PERFEVTSEL_UMASK_BRANCH_MISSES;
    break;
  case PerfEvent::fixed_instructions:
    fixed_ctrl.ctr0_enable = FIXED_CTL_RING_LVL_ALL;
    counter_offset_ = ebbrt::rdmsr(IA32_FXD_PMC(0));
    pmc_num_ = FIXED_EVT_OFFSET(0);
    break;
  case PerfEvent::fixed_cycles:
    fixed_ctrl.ctr1_enable = FIXED_CTL_RING_LVL_ALL;
    counter_offset_ = ebbrt::rdmsr(IA32_FXD_PMC(1));
    pmc_num_ = FIXED_EVT_OFFSET(1);
    break;
  case PerfEvent::fixed_reference_cycles:
    fixed_ctrl.ctr2_enable = FIXED_CTL_RING_LVL_ALL;
    counter_offset_ = ebbrt::rdmsr(IA32_FXD_PMC(2));
    pmc_num_ = FIXED_EVT_OFFSET(2);
    break;
  case PerfEvent::nil_event:
    // counter disabled
    break;
  default:
    kabort("Error: unknown performance monitor counter \n");
    break;
  };

  // Configure fixed event counter
  if (fixed_ctrl.val != 0) {
    fixed_ctrl.val |= ebbrt::rdmsr(IA32_FIXED_CTR_CTRL_MSR);
    ebbrt::wrmsr(fixed_ctrl.val, IA32_FIXED_CTR_CTRL_MSR);
    return;
  }

  // Configure general purpose counter
  if (perfevtsel.val != 0) {
    for (auto i = 0; i < pmc_count_; i++) {
      if (((pmcs >> i) & 0x1) == 0) {
        pmcs |= 1ull << i;
        pmc_num_ = i;
        perfevtsel.usermode = 1;
        perfevtsel.osmode = 1;
        perfevtsel.en = 1;
        ebbrt::wrmsr(perfevtsel.val, IA32_PERFEVTSEL_MSR(pmc_num_));
        counter_offset_ = ebbrt::rdmsr(IA32_PMC(pmc_num_));
        return;
      }
    }
    kprintf("Warning: no available hardware counters.\n");
    evt_ = PerfEvent::nil_event;
  }

  return;
}

void ebbrt::perf::PerfCounter::Clear() {
  switch (evt_) {
  case PerfEvent::nil_event:
    counter_offset_ = 0;
    break;
  case PerfEvent::fixed_instructions:
    counter_offset_ = ebbrt::rdmsr(IA32_FXD_PMC(0));
    break;
  case PerfEvent::fixed_cycles:
    counter_offset_ = ebbrt::rdmsr(IA32_FXD_PMC(1));
    break;
  case PerfEvent::fixed_reference_cycles:
    counter_offset_ = ebbrt::rdmsr(IA32_FXD_PMC(2));
    break;
  default:
    counter_offset_ = ebbrt::rdmsr(IA32_PMC(pmc_num_));
    break;
  };
  return;
}

int8_t ebbrt::perf::PerfCounter::AvailablePMCs() {
  return 0;  // pmc_count_ - next_pmc;
}

void ebbrt::perf::PerfCounter::Start() {
  if (evt_ != PerfEvent::nil_event) {
    perf_global_ctrl |= (1ull << pmc_num_);
    ebbrt::wrmsr(perf_global_ctrl, IA32_PERF_GLOBAL_CTRL_MSR);
  }
  return;
}

void ebbrt::perf::PerfCounter::Stop() {
  if (evt_ != PerfEvent::nil_event) {
    perf_global_ctrl &= ~(1ull << pmc_num_);
    ebbrt::wrmsr(perf_global_ctrl, IA32_PERF_GLOBAL_CTRL_MSR);
  }
  return;
}
uint64_t ebbrt::perf::PerfCounter::Read() {
  switch (evt_) {
  case PerfEvent::nil_event:
    return 0;
  case PerfEvent::fixed_instructions:
    return ebbrt::rdmsr((IA32_FXD_PMC(0))) - counter_offset_;
  case PerfEvent::fixed_cycles:
    return ebbrt::rdmsr((IA32_FXD_PMC(1))) - counter_offset_;
  case PerfEvent::fixed_reference_cycles:
    return ebbrt::rdmsr((IA32_FXD_PMC(2))) - counter_offset_;
  default:
    return ebbrt::rdmsr(IA32_PMC(pmc_num_)) - counter_offset_;
  };
}
