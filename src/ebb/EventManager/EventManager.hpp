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
#ifndef EBBRT_EBB_EVENTMANAGER_EVENTMANAGER_HPP
#define EBBRT_EBB_EVENTMANAGER_EVENTMANAGER_HPP

#include <functional>

#include "ebb/ebb.hpp"
#include "lrt/event_impl.hpp"

namespace ebbrt {
  class EventManager : public EbbRep {
  public:
    virtual uint8_t AllocateInterrupt(std::function<void()> func) = 0;
    virtual void Async(std::function<void()> func) = 0;
#ifdef __linux__
    virtual void RegisterFD(int fd, uint32_t events, uint8_t interrupt) = 0;
#endif
  private:
    friend void lrt::event::_event_interrupt(uint8_t interrupt);
    friend void lrt::event::process_event();
    virtual void HandleInterrupt(uint8_t interrupt) = 0;

    /** Dispatch event. */
    virtual void ProcessEvent() = 0;
  };
  const EbbRef<EventManager> event_manager =
    EbbRef<EventManager>(lrt::trans::find_static_ebb_id("EventManager"));
}
#endif
