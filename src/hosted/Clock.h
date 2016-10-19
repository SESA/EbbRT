//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_CLOCK_H_
#define HOSTED_SRC_INCLUDE_EBBRT_CLOCK_H_

#include <chrono>

namespace ebbrt {
namespace clock {

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
    auto tsc = tock_tsc();
    return std::chrono::nanoseconds(
        static_cast<uint64_t>(tsc / once.ticks_per_nano));
  }

 private:
  struct DoOnce {
    DoOnce() {
      HighResTimer timer;
      auto s = std::chrono::steady_clock::now();
      timer.tick();
      for (size_t i = 0; i < 1000000; i++) {
        asm volatile("");
      }
      auto elapsed_tsc = timer.tock_tsc();
      std::chrono::nanoseconds elapsed_steady =
          std::chrono::steady_clock::now() - s;
      ticks_per_nano = static_cast<double>(elapsed_tsc) /
                       static_cast<double>(elapsed_steady.count());
    }
    double ticks_per_nano;
  };

  static DoOnce once;

  uint64_t tock_tsc() __attribute__((always_inline)) {
    uint32_t cycles_high1, cycles_low1;
    asm volatile("rdtscp;"
                 "mov %%edx, %0;"
                 "mov %%eax, %1;"
                 "cpuid;"
                 : "=r"(cycles_high1), "=r"(cycles_low1)::"%rax", "%rbx",
                   "%rcx", "%rdx");
    auto start = (((uint64_t)cycles_high_ << 32) | cycles_low_);
    auto end = (((uint64_t)cycles_high1 << 32 | cycles_low1));
    return end - start;
  }

  uint32_t cycles_low_;
  uint32_t cycles_high_;
};

}  // namespace clock
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_CLOCK_H_
