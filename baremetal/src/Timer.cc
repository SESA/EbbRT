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
    auto now = clock::Time();
    kassert(!timers_.empty());
    while (!timers_.empty() && timers_.begin()->first <= now) {
      // we assume timers are repeating so we add the next one
      auto& kv = *timers_.begin();
      auto it =
          timers_.emplace(std::piecewise_construct,
                          std::forward_as_tuple(now + std::get<1>(kv.second)),
                          std::move(kv.second));
      timers_.erase(timers_.begin());
      std::get<0>(it->second)();
      now = clock::Time();
    }
    if (!timers_.empty()) {
      SetTimer(std::chrono::duration_cast<std::chrono::microseconds>(
          timers_.begin()->first - now));
    }
  });

  // Map timer to interrupt and enable one-shot mode
  msr::Write(msr::kX2apicLvtTimer, interrupt);
  msr::Write(msr::kX2apicDcr, 0x3);  // divide = 16

  // calibrate timer
  auto t = clock::Time();
  // set timer
  msr::Write(msr::kX2apicInitCount, 0xFFFFFFFF);

  while (clock::Time() < (t + std::chrono::milliseconds(10))) {
  }

  auto remaining = msr::Read(msr::kX2apicCurrentCount);

  // disable timer
  msr::Write(msr::kX2apicInitCount, 0);
  uint32_t elapsed = 0xFFFFFFFF;
  elapsed -= remaining;
  ticks_per_us_ = elapsed * 16 / 10000;
}

void ebbrt::Timer::SetTimer(std::chrono::microseconds from_now) {
  uint64_t ticks = from_now.count();
  ticks *= ticks_per_us_;
  // determine timer divider
  auto divider = -1;
  while (ticks > 0xFFFFFFFF) {
    ticks >>= 1;
    divider++;
  }
  kbugon(divider > 7);  // max can divide by 128 (2^7)
  uint32_t divider_set;
  if (divider == -1) {
    divider_set = 0xb;
  } else {
    divider_set = divider;
  }
  msr::Write(msr::kX2apicDcr, divider_set);
  msr::Write(msr::kX2apicInitCount, ticks);
}

void ebbrt::Timer::Start(std::chrono::microseconds timeout,
                         std::function<void()> f) {
  auto now = clock::Time();
  auto when = now + timeout;
  if (timers_.empty() || timers_.begin()->first > when) {
    SetTimer(timeout);
  }

  timers_.emplace(std::piecewise_construct,
                  std::forward_as_tuple(when),
                  std::forward_as_tuple(std::move(f), timeout));
}
