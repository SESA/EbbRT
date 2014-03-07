//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Debug.h>
#include <ebbrt/Trace.h>



#if 0
#define wrmsr(msr,lo,hi) \
asm volatile ("wrmsr" : : "c" (msr), "a" (lo), "d" (hi))
#define rdmsr(msr,lo,hi) \
asm volatile ("\trdmsr\n" : "=a" (lo), "=d" (hi) : "c" (msr))
#endif

void AppMain() { 
  
  ebbrt::trace::enable_trace();
  ebbrt::kprintf("Tracing...\n");
  ebbrt::kbugon(false);
  ebbrt::trace::disable_trace();
  ebbrt::trace::Dump();

//  ebbrt::kbugon(sizeof(ebbrt::trace::perf_event_select) != 8, "perf_event_select packing issue");
//
//  int info[4];
//  cpuid(0xA, 0, info[0], info[1], info[2], info[3]);
//  ebbrt::kprintf("EAX> %x %x %x %x %x\n",
//      (info[0] & 0xff), // version id
//      ((info[0] >> 8) & 0xff), 
//      ((info[0] >> 16) & 0xff), 
//      ((info[0] >> 24) & 0xff),
//      info[0]
//      );
//  ebbrt::kprintf("EBX> %x \n",
//      (info[1]) 
//      );
//  ebbrt::kprintf("ECX> %x \n",
//      (info[2]) 
//      );
//  ebbrt::kprintf("EDX> %x %x\n",
//      (info[3] & 0xf), 
//      ((info[3] >> 4) & 0xff)
//      );
//
//  auto time = rdtsc();
//  ebbrt::kprintf("BEGIN ============ @%llu\n", time);
//
//  //ebbrt::trace::perf_event_select event_sel;
//  // enable msr
//  //event_sel.val = 0;
//  //event_sel.eventselect_7_0 = 0x3C; // unhalted core cycles
//  //event_sel.usermode = 1;  
//  //event_sel.osmode = 1; 
//  //event_sel.en = 1; // enable
//
//  // 38DH fxed ctr ctrl
//  // 38FH perf gloabl ctrl
//  // here we enable the counter
//  ebbrt::trace::fixed_ctr_ctrl fixed_ctrl;
//  ebbrt::trace::perf_global_ctrl global_ctrl;
//
//  fixed_ctrl.val = 0;
//  fixed_ctrl.ctr0_enable = 3;
//  fixed_ctrl.ctr1_enable = 3;
//  fixed_ctrl.ctr2_enable = 3;
//  global_ctrl.val = 0;
//  global_ctrl.ctr0_enable = 1;
//  global_ctrl.ctr1_enable = 1;
//  global_ctrl.ctr2_enable = 1;
//
//  __asm__ __volatile__("wrmsr"
//                       :
//                       : "a"(fixed_ctrl.val & 0xFFFFFFFF),
//                         "d"(fixed_ctrl.val >> 32), 
//                         "c"(0x38D));
//  __asm__ __volatile__("wrmsr"
//                       :
//                       : "a"(global_ctrl.val & 0xFFFFFFFF),
//                         "d"(global_ctrl.val >> 32), 
//                         "c"(0x38F));
//
//  while ((rdtsc() - time) < 1000000)
//    ;
//
//  auto time_end = rdtsc();
//
//  uint32_t reg = 1 << 30;
//  auto count0 = rdpmc(reg);
//  auto count1 = rdpmc((reg + 1));
//  auto count2 = rdpmc((reg + 2));
//  ebbrt::kprintf("time: %llu\n", (time_end - time));
//  ebbrt::kprintf("instructions: %llu \n", count0);
//  ebbrt::kprintf("cycles_0: %llu\n", count1);
//  ebbrt::kprintf("cycles_1: %llu\n", count2);


}
