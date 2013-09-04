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
#include "ebb/SharedRoot.hpp"
#include "ebb/EventManager/EventManager.hpp"
#include "ebb/Timer/SimpleTimer.hpp"

ebbrt::EbbRoot*
ebbrt::SimpleTimer::ConstructRoot()
{
  return new SharedRoot<SimpleTimer>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol("Timer",
                        ebbrt::SimpleTimer::ConstructRoot);
}

ebbrt::SimpleTimer::SimpleTimer(EbbId id)
  : Timer{id}
{
  interrupt_ = event_manager->AllocateInterrupt([=]() {
      auto me = EbbRef<SimpleTimer>{ebbid_};
      me->FireTimers();
    });

  event_manager->RegisterFunction([=]() {
      auto me = EbbRef<SimpleTimer>{ebbid_};
      return me->CheckForInterrupt();
    });
}

void
ebbrt::SimpleTimer::Wait(std::chrono::nanoseconds duration,
                         std::function<void()> func)
{
  timers_.emplace(std::chrono::steady_clock::now() + duration,
                  std::move(func));
}

void
ebbrt::SimpleTimer::FireTimers()
{
  auto now = std::chrono::steady_clock::now();
  for (auto it = timers_.begin(); it != timers_.upper_bound(now); ++it) {
    it->second();
  }
  timers_.erase(timers_.begin(), timers_.upper_bound(now));
}

int
ebbrt::SimpleTimer::CheckForInterrupt()
{
  if (!timers_.empty() &&
      (timers_.begin()->first <= std::chrono::steady_clock::now())) {
    return interrupt_;
  }
  return -1;
}
