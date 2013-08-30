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
#ifndef EBBRT_EBB_TIMER_SIMPLETIMER_HPP
#define EBBRT_EBB_TIMER_SIMPLETIMER_HPP

#include <map>

#include "ebb/Timer/Timer.hpp"

namespace ebbrt {
  class SimpleTimer : public Timer {
  public:
    static EbbRoot* ConstructRoot();

    SimpleTimer(EbbId id);

    virtual void Wait(std::chrono::nanoseconds duration,
                      std::function<void()> func) override;
  private:
    void FireTimers();
    int CheckForInterrupt();

    std::multimap<std::chrono::steady_clock::time_point,
                  std::function<void()> > timers_;
    int interrupt_;
  };
}

#endif
