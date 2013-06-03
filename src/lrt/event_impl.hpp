#ifndef EBBRT_LRT_EVENT_IMPL_HPP
#define EBBRT_LRT_EVENT_IMPL_HPP

#include "src/lrt/event.hpp"

#ifdef LRT_ULNX
#include <src/lrt/ulnx/event_impl.hpp>
#elif LRT_BARE
#include <src/lrt/bare/event_impl.hpp>
#endif

namespace ebbrt {
  namespace lrt {
    namespace event {
      /**
       * Handle interrupt from event loop.
       */
      extern "C" void _event_interrupt(uint8_t interrupt);
    }
  }
}
#endif
