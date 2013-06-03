#ifndef EBBRT_LRT_EVENT_HPP
#define EBBRT_LRT_EVENT_HPP


#include <cstdint>

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
       * Initialize the event subsystem.
       * @param[in] num_cores The number of cores that will be
       * initialized.
       */
      void init(unsigned num_cores);

      /**
       * The core identifier.
       */
      typedef uint32_t Location;
      /**
       * Per-core event subsystem initialization
       *
       */
      void init_cpu() __attribute__((noreturn));

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

#ifdef LRT_ULNX
#include <src/lrt/ulnx/event.hpp>
#elif LRT_BARE
#include <src/lrt/bare/event.hpp>
#endif

#endif
