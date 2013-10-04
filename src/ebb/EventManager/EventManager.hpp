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
#include "ebb/EventManager/MoveFunction.hpp"
#include "lrt/config.hpp"
#include "lrt/event_impl.hpp"

namespace ebbrt {
  class EventManager : public EbbRep {
  public:
    /**
     * Allocate an interrupt which when triggered will call a function.
     * @param [in] func The function to be invoked when the interrupt fires
    */
    virtual uint8_t AllocateInterrupt(std::function<void()> func) = 0;
    /**
     * Asynchronously call a function.
     * Do not use this to continuously generate work as it may starve
     * out events
     * @param [in] func The function to be invoked
     */
    virtual void Async(move_function<void()> func) = 0;

#ifdef __linux__
    virtual void RegisterFD(int fd, uint32_t events, uint8_t interrupt) = 0;
#ifdef __bg__
    /**
     * Register a function to be called during the event loop to check
     * for an interrupt condition.
     * @param [in] The function to be called.
     *    The return value shall be the interrupt to be invoked or -1
     * if no interrupt.
     */
    virtual void RegisterFunction(std::function<int()> func) = 0;
#endif
#endif
  protected:
    EventManager(EbbId id) : EbbRep{id} {}
  private:
    friend void lrt::event::_event_interrupt(uint8_t interrupt);
    friend void lrt::event::process_event();
    virtual void HandleInterrupt(uint8_t interrupt) = 0;
    virtual void ProcessEvent() = 0;
  };
  const EbbRef<EventManager> event_manager =
    EbbRef<EventManager>(lrt::config::find_static_ebb_id(nullptr,"EventManager"));
}
#endif
