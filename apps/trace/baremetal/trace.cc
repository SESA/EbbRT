//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Debug.h>
#include <ebbrt/Trace.h>

#define ITERATIONS 100000

namespace{
const uint32_t reg = 1 << 30;  // Intel fixed-purpose-register config flag
}

void do_nothing() {
  asm volatile ("");
}

uint64_t do_rdtsc(){
  return ebbrt::trace::rdtsc();
}

uint64_t do_perf(){
  return ebbrt::trace::rdpmc((reg));
}

void AppMain() {

  uint64_t ts = 0, is = 0, cs = 0, te = 0, ie = 0, ce = 0;
  uint64_t tave, iave, cave;

  ebbrt::trace::Init();
  ebbrt::trace::Enable();


  // EXPERIMENT 1 - COST OF NOTHING
  //warmup
  for (auto i = 0; i < ITERATIONS; i++) { do_nothing(); }
  is = ebbrt::trace::rdpmc((reg));
  cs = ebbrt::trace::rdpmc((reg + 1));
  ts = ebbrt::trace::rdtsc();
  for (auto i = 0; i < ITERATIONS; i++) { do_nothing(); }
  te = ebbrt::trace::rdtsc();
  ce = ebbrt::trace::rdpmc((reg + 1));
  ie = ebbrt::trace::rdpmc((reg));
  tave = (te - ts) / ITERATIONS;
  iave = (ie - is) / ITERATIONS;
  cave = (ce - cs) / ITERATIONS;
  ebbrt::kprintf("EXP1: timeclock total:%llu ave:%llu\n",(te - ts), tave);
  ebbrt::kprintf("EXP1: instruction total:%llu ave:%llu\n",(ie - is), iave);
  ebbrt::kprintf("EXP1: cycle total:%llu ave:%llu\n", (ce - cs), cave);

  // EXPERIMENT 2 - COST OF WALLCLOCK
  // warmup
  for (auto i = 0; i < ITERATIONS; i++) { do_rdtsc(); }
  is = ebbrt::trace::rdpmc((reg));
  cs = ebbrt::trace::rdpmc((reg + 1));
  ts = ebbrt::trace::rdtsc();
  for (auto i = 0; i < ITERATIONS; i++) { do_rdtsc(); }
  te = ebbrt::trace::rdtsc();
  ce = ebbrt::trace::rdpmc((reg + 1));
  ie = ebbrt::trace::rdpmc((reg));
  tave = (te - ts) / ITERATIONS;
  iave = (ie - is) / ITERATIONS;
  cave = (ce - cs) / ITERATIONS;
  ebbrt::kprintf("EXP2: timeclock total:%llu ave:%llu\n",(te - ts), tave);
  ebbrt::kprintf("EXP2: instruction total:%llu ave:%llu\n",(ie - is), iave);
  ebbrt::kprintf("EXP2: cycle total:%llu ave:%llu\n", (ce - cs), cave);

  // EXPERIMENT 3 - COST OF PERFORMANCE COUNTERS
  // warmup
  for (auto i = 0; i < ITERATIONS; i++) { do_perf(); }
  is = ebbrt::trace::rdpmc((reg));
  cs = ebbrt::trace::rdpmc((reg + 1));
  ts = ebbrt::trace::rdtsc();
  for (auto i = 0; i < ITERATIONS; i++) { do_perf(); }
  te = ebbrt::trace::rdtsc();
  ce = ebbrt::trace::rdpmc((reg + 1));
  ie = ebbrt::trace::rdpmc((reg));
  tave = (te - ts) / ITERATIONS;
  iave = (ie - is) / ITERATIONS;
  cave = (ce - cs) / ITERATIONS;
  ebbrt::kprintf("EXP3: timeclock total:%llu ave:%llu\n",(te - ts), tave);
  ebbrt::kprintf("EXP3: instruction total:%llu ave:%llu\n",(ie - is), iave);
  ebbrt::kprintf("EXP3: cycle total:%llu ave:%llu\n", (ce - cs), cave);

#ifdef __TRACE_EXP__
  ebbrt::trace::Disable();
  ebbrt::trace::Dump();
#endif
}
