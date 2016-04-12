//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_CPUID_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_CPUID_H_

#include <cstdint>
namespace ebbrt {
namespace cpuid {

struct Result {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

struct Features {
  bool x2apic;
  bool kvm_pv_eoi;
  bool kvm_clocksource2;
};

extern Features features;

Result Cpuid(uint32_t leaf);
void Init();
}  // namespace cpuid
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_CPUID_H_
