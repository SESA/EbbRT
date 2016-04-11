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
thread_local uint8_t pmcs;  // stores allocated counters
};

thread_local uint64_t
    ebbrt::perf::perf_global_ctrl;  // stores activated counters

ebbrt::perf::PerfCounter::~PerfCounter() {
  if (evt_ == PerfEvent::nil_event) {
    return;
  }

  // stop counter
  perf_global_ctrl &= ~(1ull << pmc_num_);
  ebbrt::wrmsr(perf_global_ctrl, kIa32PerfGlobalCtrlMsr);

  // disable counter
  FixedCtrCtrl fixed_ctrl;
  PerfEvtSelMsr perfevtsel;
  perfevtsel.val = 0;
  fixed_ctrl.val |= ebbrt::rdmsr(kIa32FixedCtrCtrlMsr);

  switch (evt_) {
  case PerfEvent::nil_event:
    return;
  case PerfEvent::fixed_instructions:
    fixed_ctrl.ctr0_enable = kIa32FixedCtlRingLvlNone;
    ebbrt::wrmsr(fixed_ctrl.val, kIa32FixedCtrCtrlMsr);
    break;
  case PerfEvent::fixed_cycles:
    fixed_ctrl.ctr1_enable = kIa32FixedCtlRingLvlNone;
    ebbrt::wrmsr(fixed_ctrl.val, kIa32FixedCtrCtrlMsr);
    break;
  case PerfEvent::fixed_reference_cycles:
    fixed_ctrl.ctr2_enable = kIa32FixedCtlRingLvlNone;
    ebbrt::wrmsr(fixed_ctrl.val, kIa32FixedCtrCtrlMsr);
    break;
  default:
    perfevtsel.usermode = 0;
    perfevtsel.osmode = 0;
    perfevtsel.en = 0;
    ebbrt::wrmsr(perfevtsel.val, kIa32PerfEvtSelMsr(pmc_num_));
    pmcs &= ~(1ull << pmc_num_);
    break;
  }
}

ebbrt::perf::PerfCounter::PerfCounter(ebbrt::perf::PerfEvent evt) : evt_{evt} {
  auto r = ebbrt::cpuid::Cpuid(kPerfCpuLeaf);
  auto pmc_version = r.eax & 0xFF;
  auto pmc_count = (r.eax >> 8) & 0xFF;
  auto pmc_events = (r.ebx) & 0xFF;
  auto evt_num = static_cast<uint8_t>(evt_);

  // check if PMC are supported on this architecture
  if (pmc_version == 0) {
    kprintf("Warning: performance monitoring counters are not supported.\n");
    evt_ = PerfEvent::nil_event;
  } else if (((pmc_events >> evt_num) & 0x1) == 1) {
    kprintf("Warning: event type (#%d) is not supported.\n");
    evt_ = PerfEvent::nil_event;
  }

  FixedCtrCtrl fixed_ctrl;
  PerfEvtSelMsr perfevtsel;
  perfevtsel.val = 0;
  fixed_ctrl.val = 0;

  switch (evt_) {
  case PerfEvent::tlb_store_misses:
    perfevtsel.event = kPerfEvtSelEvtTlbStoreMisses;
    perfevtsel.umask = kPerfEvtSelUmaskTlbStoreMisses;
    break;
  case PerfEvent::tlb_load_misses:
    perfevtsel.event = kPerfEvtSelEvtTlbLoadMisses;
    perfevtsel.umask = kPerfEvtSelUmaskTlbLoadMisses;
    break;
  case PerfEvent::cycles:
    perfevtsel.event = kPerfEvtSelEvtCycles;
    perfevtsel.umask = kPerfEvtSelUmaskCycles;
    break;
  case PerfEvent::instructions:
    perfevtsel.event = kPerfEvtSelEvtInstructions;
    perfevtsel.umask = kPerfEvtSelUmaskInstructions;
    break;
  case PerfEvent::reference_cycles:
    perfevtsel.event = kPerfEvtSelEvtCyclesRef;
    perfevtsel.umask = kPerfEvtSelUmaskCyclesRef;
    break;
  case PerfEvent::llc_references:
    perfevtsel.event = kPerfEvtSelEvtLLCRef;
    perfevtsel.umask = kPerfEvtSelUmaskLLCRef;
    break;
  case PerfEvent::llc_misses:
    perfevtsel.event = kPerfEvtSelEvtLLCMisses;
    perfevtsel.umask = kPerfEvtSelUmaskLLCMisses;
    break;
  case PerfEvent::branch_instructions:
    perfevtsel.event = kPerfEvtSelEvtBranchInstructions;
    perfevtsel.umask = kPerfEvtSelUmaskBranchInstructions;
    break;
  case PerfEvent::branch_misses:
    perfevtsel.event = kPerfEvtSelEvtBranchMisses;
    perfevtsel.umask = kPerfEvtSelUmaskBranchMisses;
    break;
  case PerfEvent::fixed_instructions:
    fixed_ctrl.ctr0_enable = kIa32FixedCtlRingLvlAll;
    counter_offset_ = ebbrt::rdmsr(kIa32FxdPmc(0));
    pmc_num_ = kFixedEvtOffset(0);
    break;
  case PerfEvent::fixed_cycles:
    fixed_ctrl.ctr1_enable = kIa32FixedCtlRingLvlAll;
    counter_offset_ = ebbrt::rdmsr(kIa32FxdPmc(1));
    pmc_num_ = kFixedEvtOffset(1);
    break;
  case PerfEvent::fixed_reference_cycles:
    fixed_ctrl.ctr2_enable = kIa32FixedCtlRingLvlAll;
    counter_offset_ = ebbrt::rdmsr(kIa32FxdPmc(2));
    pmc_num_ = kFixedEvtOffset(2);
    break;
  case PerfEvent::nil_event:
    // counter disabled
    break;
  default:
    kabort("Error: unknown performance monitor counter \n");
    break;
  }

  // Configure fixed event counter
  if (fixed_ctrl.val != 0) {
    fixed_ctrl.val |= ebbrt::rdmsr(kIa32FixedCtrCtrlMsr);
    ebbrt::wrmsr(fixed_ctrl.val, kIa32FixedCtrCtrlMsr);
    kprintf("Perf fixed counter #%d initialized to evt=%u\n", pmc_num_,
            static_cast<uint8_t>(evt_));
    return;
  }

  // Configure general purpose counter
  if (perfevtsel.val != 0) {
    for (auto i = 0u; i < pmc_count; i++) {
      if (((pmcs >> i) & 0x1) == 0) {
        pmc_num_ = i;
        pmcs |= (0x1u << i);
        kprintf("DEBUG#%d %x \n", pmc_num_, pmcs);
        perfevtsel.usermode = 1;
        perfevtsel.osmode = 1;
        perfevtsel.en = 1;
        ebbrt::wrmsr(perfevtsel.val, kIa32PerfEvtSelMsr(pmc_num_));
        counter_offset_ = ebbrt::rdmsr(kIa32Pmc(pmc_num_));
        kprintf("Perf counter #%d initialized to evt=%u\n", pmc_num_,
                static_cast<uint8_t>(evt_));
        return;
      }
    }
    kprintf("Warning: no available hardware counters.\n");
    evt_ = PerfEvent::nil_event;
  }
}

void ebbrt::perf::PerfCounter::Clear() {
  switch (evt_) {
  case PerfEvent::nil_event:
    counter_offset_ = 0;
    break;
  case PerfEvent::fixed_instructions:
    counter_offset_ = ebbrt::rdmsr(kIa32FxdPmc(0));
    break;
  case PerfEvent::fixed_cycles:
    counter_offset_ = ebbrt::rdmsr(kIa32FxdPmc(1));
    break;
  case PerfEvent::fixed_reference_cycles:
    counter_offset_ = ebbrt::rdmsr(kIa32FxdPmc(2));
    break;
  default:
    counter_offset_ = ebbrt::rdmsr(kIa32Pmc(pmc_num_));
    break;
  }
  return;
}

uint64_t ebbrt::perf::PerfCounter::Read() {
  switch (evt_) {
  case PerfEvent::nil_event:
    return 0;
  case PerfEvent::fixed_instructions:
    return ebbrt::rdmsr((kIa32FxdPmc(0))) - counter_offset_;
  case PerfEvent::fixed_cycles:
    return ebbrt::rdmsr((kIa32FxdPmc(1))) - counter_offset_;
  case PerfEvent::fixed_reference_cycles:
    return ebbrt::rdmsr((kIa32FxdPmc(2))) - counter_offset_;
  default:
    return ebbrt::rdmsr(kIa32Pmc(pmc_num_)) - counter_offset_;
  }
}
