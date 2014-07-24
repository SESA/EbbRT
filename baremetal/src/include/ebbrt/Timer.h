//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_TIMER_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_TIMER_H_

#include <chrono>

#include <boost/intrusive/set.hpp>

#include <ebbrt/MulticoreEbbStatic.h>
#include <ebbrt/Trans.h>

namespace ebbrt {

class Timer : public MulticoreEbbStatic<Timer> {
 public:
  class Hook : public boost::intrusive::set_base_hook<> {
   public:
    virtual ~Hook() {}
    virtual void Fire() = 0;

    bool operator==(const Hook& rhs) const {
      return fire_time_ == rhs.fire_time_;
    }

    bool operator!=(const ebbrt::Timer::Hook& rhs) const {
      return !(*this == rhs);
    }

    bool operator<(const Hook& rhs) const {
      return fire_time_ < rhs.fire_time_;
    }

    bool operator>(const ebbrt::Timer::Hook& rhs) const { return rhs < *this; }

    bool operator<=(const ebbrt::Timer::Hook& rhs) const {
      return !(*this > rhs);
    }

    bool operator>=(const ebbrt::Timer::Hook& rhs) const {
      return !(*this < rhs);
    }

   private:
    std::chrono::nanoseconds fire_time_;
    std::chrono::microseconds repeat_us_;

    friend Timer;
  };

  static const constexpr EbbId static_id = kTimerId;

  Timer();

  void Start(Hook&, std::chrono::microseconds timeout, bool repeat);
  void Stop(Hook&);

 private:
  void SetTimer(std::chrono::microseconds from_now);
  void StopTimer();

  uint64_t ticks_per_us_;
  boost::intrusive::multiset<Hook> timers_;
};

const constexpr auto timer = EbbRef<Timer>(Timer::static_id);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_TIMER_H_
