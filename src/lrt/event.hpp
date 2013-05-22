#ifndef EBBRT_LRT_EVENT_HPP
#define EBBRT_LRT_EVENT_HPP


#include <cstdint>

namespace ebbrt {
  namespace lrt {
    namespace event {
      /**
       * @brief Initial event configuration
       *
       * @param num_cores
       *
       * @return 
       */
      bool init(unsigned num_cores);
      typedef uint32_t Location;
      /**
       * @brief Per-core event configuration
       *
       */
      void init_cpu() __attribute__((noreturn));

      /**
       * @brief Push onto alt-stack
       *
       * @param val
       *
       * @return 
       */
      extern "C" void _event_altstack_push(uintptr_t val);
      /**
       * @brief Pop from the alt-stack
       *
       * @return 
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
