//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Timer.h>

#include <ebbrt/Clock.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/Msr.h>

const constexpr ebbrt::EbbId ebbrt::Timer::static_id;

ebbrt::Timer::Timer() {
  auto interrupt = event_manager->AllocateVector([this]() {
    auto now = clock::Wall::Now().time_since_epoch();
    while (!timers_.empty() && timers_.begin()->fire_time_ <= now) {
      auto& hook = *timers_.begin();

      // remove from the set
      timers_.erase(hook);

      // If it needs repeating, put it back in with the updated time
      if (hook.repeat_us_ != std::chrono::microseconds::zero()) {
        hook.fire_time_ = now + hook.repeat_us_;
        timers_.insert(hook);
      }

      hook.Fire();

      now = clock::Wall::Now().time_since_epoch();
    }
    if (!timers_.empty()) {
      SetTimer(std::chrono::duration_cast<std::chrono::microseconds>(
          timers_.begin()->fire_time_ - now));
    }
  });

  // Map timer to interrupt and enable one-shot mode
  msr::Write(msr::kX2apicLvtTimer, interrupt);
  msr::Write(msr::kX2apicDcr, 0x3);  // divide = 16

  // calibrate timer
  auto t = clock::Wall::Now().time_since_epoch();
  // set timer
  msr::Write(msr::kX2apicInitCount, 0xFFFFFFFF);

  while (clock::Wall::Now().time_since_epoch() <
         (t + std::chrono::milliseconds(10))) {
  }

  auto remaining = msr::Read(msr::kX2apicCurrentCount);

  // disable timer
  msr::Write(msr::kX2apicInitCount, 0);
  uint32_t elapsed = 0xFFFFFFFF;
  elapsed -= remaining;
  ticks_per_us_ = elapsed * 16 / 10000;
}

void ebbrt::Timer::SetTimer(std::chrono::microseconds from_now) {
  if (unlikely(from_now.count() == 0)) {
    msr::Write(msr::kX2apicDcr, 0xb);
    msr::Write(msr::kX2apicInitCount, 1);
    return;
  }
  uint64_t ticks = from_now.count();
  ticks *= ticks_per_us_;
  // determine timer divider
  auto divider = -1;
  while (ticks > 0xFFFFFFFF) {
    ticks >>= 1;
    divider++;
  }
  if (divider > 7) {
    divider = 7;
    ticks = 0xFFFFFFFF;
  }
  uint32_t divider_set;
  if (divider == -1) {
    divider_set = 0xb;
  } else {
    divider_set = divider;
  }
  kassert(ticks > 0);
  msr::Write(msr::kX2apicDcr, divider_set);
  msr::Write(msr::kX2apicInitCount, ticks);
}

void ebbrt::Timer::StopTimer() { msr::Write(msr::kX2apicInitCount, 0); }

void ebbrt::Timer::Start(Hook& hook, std::chrono::microseconds timeout,
                         bool repeat) {
  auto now = clock::Wall::Now().time_since_epoch();
  hook.fire_time_ = now + timeout;
  if (timers_.empty() || *timers_.begin() > hook) {
    SetTimer(timeout);
  }

  hook.repeat_us_ = repeat ? timeout : std::chrono::microseconds::zero();
  timers_.insert(hook);
}

void ebbrt::Timer::Stop(Hook& hook) {
  timers_.erase(hook);

  if (timers_.empty()) {
    StopTimer();
  } else {
    auto now = clock::Wall::Now().time_since_epoch();
    auto fire_time = timers_.begin()->fire_time_;
    if (now < fire_time) {
      SetTimer(std::chrono::duration_cast<std::chrono::microseconds>(fire_time -
                                                                     now));
    }
  }
}
