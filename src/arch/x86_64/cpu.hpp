#ifndef EBBRT_ARCH_CPU_HPP
#error "Don't include this file directly"
#endif

#include <cstdint>

namespace ebbrt {
  const uint32_t CPUID_FEATURES = 1;

  // CPUID FEATURE FLAGS
  const uint32_t CPUID_EDX_HAS_LAPIC = 1 << 9;

  static inline void
  cpuid(uint32_t index, uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
        uint32_t *edx)
  {
    __asm__ (
             "cpuid"
             : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
             : "a" (index)
             );
  }

  const uint32_t MSR_APIC_BASE = 0x1b;
  const uint32_t MSR_FS_BASE = 0xC0000100;

  static inline void
  wrmsr(uint32_t msr, uint64_t val) {
    asm volatile ("wrmsr"
                  :
                  : "d" ((uint32_t)(val >> 32)),
                    "a" ((uint32_t)(val & 0xFFFFFFFF)),
                    "c" (msr)
                  );
  }

  static inline uint64_t
  rdmsr(uint32_t msr) {
    uint32_t low, high;
    asm volatile ("rdmsr"
                  : "=a" (low), "=d" (high)
                  : "c" (msr)
                  );
    return (static_cast<uint64_t>(high) << 32) | low;
  }

  static inline uint64_t
  rdtsc()
  {
    uint32_t low, high;
    asm volatile ("rdtsc"
                  : "=a" (low), "=d" (high));
    return (uint64_t)high << 32 | low;
  }
}
