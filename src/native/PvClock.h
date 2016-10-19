//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_PVCLOCK_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_PVCLOCK_H_

#include "Clock_internal.h"

namespace ebbrt {
namespace clock {
class PvClock : public Clock {
 public:
  static PvClock* GetClock();

  PvClock();

  void ApInit() override;
  Wall::time_point Now() noexcept override;
  std::chrono::nanoseconds Uptime() noexcept override;
  std::chrono::nanoseconds TscToNano(uint64_t tsc) noexcept override;
};
}  // namespace clock
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_PVCLOCK_H_
