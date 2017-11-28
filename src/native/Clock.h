//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_CLOCK_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_CLOCK_H_

#include <chrono>

namespace ebbrt {
namespace clock {
void Init();
void ApInit();

class Wall {
 public:
  typedef std::chrono::nanoseconds duration;
  typedef std::chrono::time_point<ebbrt::clock::Wall> time_point;

  static time_point Now() noexcept;
  // To keep compatability with standard clock interfaces
  static time_point now() { return Now(); }
};

std::chrono::nanoseconds Uptime() noexcept;
std::chrono::nanoseconds TscToNano(uint64_t tsc) noexcept;
void SleepMilli(uint32_t t);

class HighResTimer {
 public:
  void tick() __attribute__((always_inline)) {
    asm volatile("cpuid;"
                 "rdtsc;"
                 "mov %%edx, %0;"
                 "mov %%eax, %1;"
                 : "=r"(cycles_high_), "=r"(cycles_low_)::"%rax", "%rbx",
                   "%rcx", "%rdx");
  }
  std::chrono::nanoseconds tock() __attribute__((always_inline)) {
    uint32_t cycles_high1, cycles_low1;
    asm volatile("rdtscp;"
                 "mov %%edx, %0;"
                 "mov %%eax, %1;"
                 "cpuid;"
                 : "=r"(cycles_high1), "=r"(cycles_low1)::"%rax", "%rbx",
                   "%rcx", "%rdx");
    auto start = (((uint64_t)cycles_high_ << 32) | cycles_low_);
    auto end = (((uint64_t)cycles_high1 << 32 | cycles_low1));
    auto sample = end - start;
    return TscToNano(sample);
  }

 private:
  uint32_t cycles_low_;
  uint32_t cycles_high_;
};
}  // namespace clock
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_CLOCK_H_
