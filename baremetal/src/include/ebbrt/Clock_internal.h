//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_CLOCK_INTERNAL_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_CLOCK_INTERNAL_H_

#include <ebbrt/Clock.h>

namespace ebbrt {
namespace clock {
class Clock {
 public:
  virtual void ApInit() = 0;
  virtual Wall::time_point Now() noexcept = 0;
  virtual std::chrono::nanoseconds Uptime() noexcept = 0;
  virtual std::chrono::nanoseconds TscToNano(uint64_t tsc) noexcept = 0;
};
}
}  // namespace ebbrt
#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_CLOCK_INTERNAL_H_
