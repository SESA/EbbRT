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
#ifndef EBBRT_LRT_EVENT_HPP
#define EBBRT_LRT_EVENT_HPP

#include <cstdint>

#ifdef LRT_ULNX
#include <src/lrt/ulnx/event.hpp>
#elif LRT_BARE
#include <src/lrt/bare/event.hpp>
#endif

namespace ebbrt {
  namespace lrt {
    /**
     * The subsystem responsible for managing execution on a core or
     * thread.
     * The system is event driven and so we refer to each thread of
     * execution as an event which will not be preempted.
     */
    namespace event {
      /**
       * Push a value onto the alternate stack
       * @param [in] val The word to be pushed onto the stack
       */
      extern "C" void _event_altstack_push(uintptr_t val);
      /**
       * Pop a value off the alternate stack
       *
       * @return The word on top of the stack
       */
      extern "C" uintptr_t _event_altstack_pop();
    }
  }
}

#endif
