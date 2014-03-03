//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ctime>
#include <cstdint>
#include <ebbrt/Debug.h>

//__attribute__((no_instrument_function))
namespace ebbrt {
namespace trace {

const int MAX_TRACE = 100000;
struct trace_entry {
  bool enter;
  uintptr_t func;
  uintptr_t caller;
  uint64_t time;
} trace_log[MAX_TRACE];

struct trace_entry* trace_current_ptr = trace_log;

bool trace_enabled;
int trace_count = 0;  // temporary

void enable_trace() { trace_enabled = true; }
void disable_trace() { trace_enabled = false; }

void Init() { return; }

void Dump() {
  kprintf("BEGIN TRACE DUMP ============ \n");
  for (int i = 0; i < trace_count; i++)
    kprintf("%d %p %p %llu\n", trace_log[i].enter, trace_log[i].func,
           trace_log[i].caller, trace_log[i].time);
  kprintf("END TRACE DUMP ============ \n");
}

inline uint64_t
rdtsc(void) 
{
  uint32_t a,d;
  __asm__ __volatile__("rdtsc" : "=a" (a), "=d" (d));
  return ((uint64_t)a) | (((uint64_t)d) << 32);
}


extern "C" void __cyg_profile_func_enter(void* func, void* caller) {

  if (trace_enabled && (trace_count < MAX_TRACE)) {
      auto entry = trace_current_ptr;
      entry->enter = true;
      entry->func = reinterpret_cast<uintptr_t>(func);
      entry->caller = reinterpret_cast<uintptr_t>(caller);
      entry->time = rdtsc();
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
      trace_current_ptr++;
      trace_count++;
    }
    return;
}
}
}
