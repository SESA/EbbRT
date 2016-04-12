//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <cstdint>
#include <ctime>
#include <ebbrt/Clock.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/Debug.h>
#include <ebbrt/Trace.h>
#include <string.h>

namespace {
const constexpr size_t MAX_TRACE = 1000000;
ebbrt::trace::trace_entry trace_log[MAX_TRACE];
const uint32_t reg = 1 << 30;  // Intel fixed-purpose-register config flag
bool global_trace_enabled;
thread_local bool local_trace_enabled;
bool is_intel;
uint32_t trace_count = 0;
}

void ebbrt::trace::Init() {
  int x;
  char info[13];
  cpuid(0, 0, reinterpret_cast<uint32_t*>(&x),
        reinterpret_cast<uint32_t*>(&info[0]),
        reinterpret_cast<uint32_t*>(&info[8]),
        reinterpret_cast<uint32_t*>(&info[4]));
  info[12] = '\0';
  if (strcmp(info, "GenuineIntel") == 0)
    is_intel = true;
  else
    kprintf("KVM trace virtualization not fully supported on %s\n", info);

  if (is_intel) {
    fixed_ctr_ctrl fixed_ctrl;
    perf_global_ctrl global_ctrl;
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
  global_trace_enabled = true;
  return;
}

void ebbrt::trace::AddNote(std::string label) {
  if (global_trace_enabled) {
    if (local_trace_enabled) {
      trace_log[trace_count].status = 100;
      std::size_t len = label.copy(trace_log[trace_count].point, 40, 0);
      trace_log[trace_count].point[len] = '\0';
      trace_count++;
    }
  }
}

void ebbrt::trace::AddTracepoint(uint8_t status) {
  if (global_trace_enabled) {
    if (local_trace_enabled) {
      if (ebbrt::Cpu::GetMine() == 0) {
        auto func = __builtin_return_address(0);
        trace_log[trace_count].status = status;
        trace_log[trace_count].func = reinterpret_cast<uintptr_t>(func);
        trace_log[trace_count].caller = 0;
        trace_log[trace_count].time = ebbrt::trace::rdtsc();
        if (is_intel) {
          trace_log[trace_count].instructions = ebbrt::trace::rdpmc((reg));
          trace_log[trace_count].cycles = ebbrt::trace::rdpmc((reg + 1));
        }
        trace_count++;
      }
    }
  }
}

void ebbrt::trace::AddTimestamp(uint8_t status) {
  if (global_trace_enabled) {
    if (local_trace_enabled) {
      if (ebbrt::Cpu::GetMine() == 0) {
        auto func = __builtin_return_address(0);
        trace_log[trace_count].status = status;
        trace_log[trace_count].func = reinterpret_cast<uintptr_t>(func);
        trace_log[trace_count].caller = 0;

        auto tp = clock::Wall::Now();
        auto dur = tp.time_since_epoch();
        auto dur_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(dur);
        trace_log[trace_count].time = dur_ns.count();
        trace_count++;
      }
    }
  }
}

void ebbrt::trace::Enable() {
  if (global_trace_enabled) {
    if (ebbrt::Cpu::GetMine() == 0) {
      local_trace_enabled = true;
      if (is_intel) {
        perf_global_ctrl global_ctrl;
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
  }
}

void ebbrt::trace::Disable() {
  if (global_trace_enabled) {
    if (ebbrt::Cpu::GetMine() == 0) {
      local_trace_enabled = false;
      if (is_intel) {
        perf_global_ctrl global_ctrl;
        global_ctrl.val = 0;
        __asm__ __volatile__("wrmsr"
                             :
                             : "a"(global_ctrl.val & 0xFFFFFFFF),
                               "d"(global_ctrl.val >> 32), "c"(0x38F));
      }
    }
  }
}

void ebbrt::trace::Dump() {
  kprintf("================================ \n");
  kprintf(" BEGIN TRACE DUMP (%llu)\n", trace_count);
  kprintf("================================ \n");
  for (uint32_t i = 0; i < trace_count; i++)
    if (trace_log[i].status == 2)
      kprintf("TRACEPOINT: %s\n", trace_log[i].point);
    else
      kprintf("%d %p %p %llu %llu %llu\n", trace_log[i].status,
              trace_log[i].func, trace_log[i].caller, trace_log[i].time,
              trace_log[i].cycles, trace_log[i].instructions);
  kprintf("================================ \n");
  kprintf(" END TRACE DUMP (%llu)\n", trace_count);
  kprintf("================================ \n");
}

extern "C" void __cyg_profile_func_enter(void* func, void* caller) {
  // Make sure to use the `no_instrument_function` attribute for any calls
  // withing this function
  if (global_trace_enabled) {
    if (local_trace_enabled) {
      if (ebbrt::Cpu::GetMine() == 0) {
        trace_log[trace_count].status = 1;
        trace_log[trace_count].func = reinterpret_cast<uintptr_t>(func);
        trace_log[trace_count].caller = reinterpret_cast<uintptr_t>(caller);
        trace_log[trace_count].time = ebbrt::trace::rdtsc();
        if (is_intel) {
          trace_log[trace_count].instructions = ebbrt::trace::rdpmc((reg));
          trace_log[trace_count].cycles = ebbrt::trace::rdpmc((reg + 1));
        }
        trace_count++;
      }
    }
  }
  return;
}

extern "C" void __cyg_profile_func_exit(void* func, void* caller) {
  // Make sure to use the `no_instrument_function` attribute for any calls
  // withing this function
  if (global_trace_enabled) {
    if (local_trace_enabled) {
      if (ebbrt::Cpu::GetMine() == 0) {
        trace_log[trace_count].status = 0;
        trace_log[trace_count].func = reinterpret_cast<uintptr_t>(func);
        trace_log[trace_count].caller = reinterpret_cast<uintptr_t>(caller);
        trace_log[trace_count].time = ebbrt::trace::rdtsc();
        if (is_intel) {
          trace_log[trace_count].instructions = ebbrt::trace::rdpmc((reg));
          trace_log[trace_count].cycles = ebbrt::trace::rdpmc((reg + 1));
        }
        trace_count++;
      }
    }
  }
  return;
}
