//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ctime>
#include <cstdint>
#include <string.h>
#include <ebbrt/Trace.h>
#include <ebbrt/Debug.h>

//__attribute__((no_instrument_function))
namespace ebbrt {
namespace trace {

#if 0
#define wrmsr(msr,lo,hi) \
asm volatile ("wrmsr" : : "c" (msr), "a" (lo), "d" (hi))
#define rdmsr(msr,lo,hi) \
asm volatile ("\trdmsr\n" : "=a" (lo), "=d" (hi) : "c" (msr))
#endif
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


const int MAX_TRACE = 100000;
trace_entry trace_log[MAX_TRACE];
trace_entry* trace_current_ptr = trace_log;

bool trace_enabled;
bool is_intel = false;
int trace_count = 0;  // temporary
uint32_t reg = 1 << 30;  // fixed-purpose register flag

void enable_trace() {
  trace_enabled = true;
  if (is_intel) {
    ebbrt::trace::perf_global_ctrl global_ctrl;
    global_ctrl.val = 0;
    global_ctrl.ctr0_enable = 1;
    global_ctrl.ctr1_enable = 1;
    global_ctrl.ctr2_enable = 1;
    __asm__ __volatile__("wrmsr"
                         :
                         : "a"(global_ctrl.val & 0xFFFFFFFF),
                           "d"(global_ctrl.val >> 32), "c"(0x38F));
  }
}
void disable_trace() {
  trace_enabled = false;
  if (is_intel) {
    ebbrt::trace::perf_global_ctrl global_ctrl;
    global_ctrl.val = 0;
    __asm__ __volatile__("wrmsr"
                         :
                         : "a"(global_ctrl.val & 0xFFFFFFFF),
                           "d"(global_ctrl.val >> 32), "c"(0x38F));
  }
}

void Init() {

  char info[13];
  int x=1;
  cpuid(0, 0, reinterpret_cast<uint32_t*>(&x),
        reinterpret_cast<uint32_t*>(&info[0]),
        reinterpret_cast<uint32_t*>(&info[8]),
        reinterpret_cast<uint32_t*>(&info[4]));
  info[12] = '\0';
  if (strcmp(info, "GenuineIntel") == 0)
    is_intel = true;
  else
    kprintf("Trace not fully supported on %s\n", info);

  if (is_intel) {
    ebbrt::trace::fixed_ctr_ctrl fixed_ctrl;
    ebbrt::trace::perf_global_ctrl global_ctrl;
    fixed_ctrl.val = 0;
    fixed_ctrl.ctr0_enable = 3;
    fixed_ctrl.ctr1_enable = 3;
    fixed_ctrl.ctr2_enable = 3;
    global_ctrl.val = 0;
    __asm__ __volatile__("wrmsr"
                         :
                         : "a"(fixed_ctrl.val & 0xFFFFFFFF),
                           "d"(fixed_ctrl.val >> 32), "c"(0x38D));
    __asm__ __volatile__("wrmsr"
                         :
                         : "a"(global_ctrl.val & 0xFFFFFFFF),
                           "d"(global_ctrl.val >> 32), "c"(0x38F));
  }
  return; 
}

void Dump() {
  kprintf("BEGIN TRACE DUMP ============ \n");
  for (int i = 0; i < trace_count; i++)
    if (is_intel) {
      kprintf("%d %p %p %llu %llu %llu\n", trace_log[i].enter,
              trace_log[i].func, trace_log[i].caller, trace_log[i].time,
              trace_log[i].cycles, trace_log[i].instructions);
    }else{
      kprintf("%d %p %p %llu %llu %llu\n", trace_log[i].enter,
              trace_log[i].func, trace_log[i].caller, trace_log[i].time,
              0,0);
    }
  kprintf("END TRACE DUMP ============ \n");
}


extern "C" void __cyg_profile_func_enter(void* func, void* caller) {
  if (trace_enabled && (trace_count < MAX_TRACE)) {
    auto entry = trace_current_ptr;
    entry->enter = true;
    entry->func = reinterpret_cast<uintptr_t>(func);
    entry->caller = reinterpret_cast<uintptr_t>(caller);
    entry->time = rdtsc();
    if (is_intel) {
      entry->instructions = rdpmc((reg));
      entry->cycles = rdpmc((reg + 1));
    }
    trace_current_ptr++;
    trace_count++;
  }
  return;
}

extern "C" void __cyg_profile_func_exit(void* func, void* caller) {
  if (trace_enabled && (trace_count < MAX_TRACE)) {
    auto entry = trace_current_ptr;
    entry->enter = false;
    entry->func = reinterpret_cast<uintptr_t>(func);
    entry->caller = reinterpret_cast<uintptr_t>(caller);
    entry->time = rdtsc();
    if (is_intel) {
      entry->instructions = rdpmc((reg));
      entry->cycles = rdpmc((reg + 1));
    }
    trace_current_ptr++;
    trace_count++;
  }
  return;
}
}
}
