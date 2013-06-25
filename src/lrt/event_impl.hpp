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
#ifndef EBBRT_LRT_EVENT_IMPL_HPP
#define EBBRT_LRT_EVENT_IMPL_HPP

#include "src/lrt/event.hpp"

#ifdef LRT_BARE
#include <src/lrt/bare/event_impl.hpp>
#endif

namespace ebbrt {
  namespace lrt {
    namespace event {
      /**
       * Handle interrupt from event loop.
       */
      extern "C" void _event_interrupt(uint8_t interrupt);

      /**
       * Dispatch one event.
       */
      extern "C" void process_event();
    }
  }
}
#endif
