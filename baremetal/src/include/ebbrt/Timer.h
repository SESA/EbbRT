//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_TIMER_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_TIMER_H_
#pragma once

#include <chrono>
#include <map>

#include <ebbrt/MulticoreEbbStatic.h>
#include <ebbrt/Trans.h>

namespace ebbrt {

class Timer : public MulticoreEbbStatic<Timer> {
 public:
  static const constexpr EbbId static_id = kTimerId;

  Timer();

  void Start(std::chrono::microseconds timeout, std::function<void()> f);

 private:
  void SetTimer(std::chrono::microseconds from_now);

  uint64_t ticks_per_us_;
  std::multimap<std::chrono::nanoseconds,
                std::tuple<std::function<void()>, std::chrono::microseconds> >
      timers_;
};

const constexpr auto timer = EbbRef<Timer>(Timer::static_id);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_TIMER_H_
