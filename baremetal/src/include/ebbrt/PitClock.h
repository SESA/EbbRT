//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_PITCLOCK_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_PITCLOCK_H_

#include <ebbrt/Clock_internal.h>

namespace ebbrt {
namespace clock {
class PitClock : public Clock {
 public:
  static PitClock* GetClock();

  PitClock();

  void ApInit() override;
  Wall::time_point Now() noexcept override;
  std::chrono::nanoseconds Uptime() noexcept override;
  std::chrono::nanoseconds TscToNano(uint64_t tsc) noexcept override;
};
}  // namespace clock
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_PITCLOCK_H_
