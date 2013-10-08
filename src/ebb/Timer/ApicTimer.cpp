/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "app/app.hpp"
#include "arch/x86_64/apic.hpp"
#include "arch/x86_64/acpi.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/EventManager/EventManager.hpp"
#include "ebb/Timer/ApicTimer.hpp"


#include <inttypes.h>

ebbrt::EbbRoot*
ebbrt::ApicTimer::ConstructRoot()
{
  return new SharedRoot<ApicTimer>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol("Timer",
                        ebbrt::ApicTimer::ConstructRoot);
}

#include <cstdio>

ebbrt::ApicTimer::ApicTimer(EbbId id) : Timer{id}
{
  auto interrupt = event_manager->AllocateInterrupt([=]() {
      auto me = EbbRef<ApicTimer>{ebbid_};
      me->FireTimers();
    });

  //Map timer to interrupt and enable one-shot mode
  apic::lapic->lvt_timer = interrupt;
  apic::lapic->dcr = 0x3; //divide = 16

  auto timer = acpi::get_timer();

  //Timer Calibration:
  //we want to wait ~10ms and see how many ticks the Apic went through
  //The acpi pm timer runs at 3.579545 Mhz.
  // So 3.579545 MHz / 100 Hz ~ 35795 pm timer ticks / 10ms =
  // 0x8Bd3 ticks per 10 ms
  uint32_t t1 = in32(timer);
  t1 += 0x8bd3;

  //set apic timer
  apic::lapic->init_count = 0xFFFFFFFF;

  // FIXME: deal with wrap
  while (in32(timer) < t1)
    ;

  auto remaining = apic::lapic->current_count;

  //disable apic timer
  apic::lapic->init_count = 0;

  //compute
  uint32_t elapsed = 0xFFFFFFFF;
  elapsed -= remaining;
  ticks_per_us_ = elapsed * 16 / 10000;
}


namespace {
  void SetTimer(std::chrono::microseconds from_now, uint32_t ticks_per_us)
  {
    uint64_t ticks = from_now.count();
    ticks *= ticks_per_us;
    auto divider = -1;
    while (ticks > 0xFFFFFFFF) {
      ticks >>= 1;
      divider++;
    }
    LRT_ASSERT(divider <= 7); //max can divide by 128 (2^7)
    uint32_t divider_set;
    if (divider == -1) {
      divider_set = 0xB;
    } else {
      divider_set = divider;
    }
    ebbrt::apic::lapic->dcr = divider_set;
    ebbrt::apic::lapic->init_count = ticks;
  }
}

void
ebbrt::ApicTimer::Wait(std::chrono::microseconds duration,
                         std::function<void()> func)
{
  if (timers_.empty()) {
    SetTimer(duration, ticks_per_us_);
    timers_.emplace(duration, std::move(func));
  } else {
    auto remaining = apic::lapic->current_count;
    auto remaining_us = remaining / ticks_per_us_;
    if (remaining_us > duration.count()) {
      SetTimer(duration, ticks_per_us_);
    }
    auto fire_after_head =
      (timers_.top().first - std::chrono::microseconds{remaining_us}) +
      duration;
    timers_.emplace(fire_after_head, std::move(func));
  }
}

void
ebbrt::ApicTimer::FireTimers()
{
  auto time = timers_.top().first;
  std::vector<std::function<void()> > funcs;
  while (!timers_.empty() && timers_.top().first == time) {
    funcs.emplace_back(std::move(timers_.top().second));
    timers_.pop();
  }

  if (!timers_.empty()) {
    SetTimer(timers_.top().first - time, ticks_per_us_);
  }

  for (const auto& func : funcs) {
    func();
  }
}
