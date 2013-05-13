#ifndef EBBRT_LRT_EVENT_HPP
#define EBBRT_LRT_EVENT_HPP

#include <cstdint>

namespace ebbrt {
  namespace lrt {
    namespace event {
      bool init(unsigned num_cores);
      typedef uint32_t Location;
      void init_cpu() __attribute__((noreturn));

      extern "C" void _event_altstack_push(uintptr_t val);
      extern "C" uintptr_t _event_altstack_pop();
    }
  }
}

#include "lrt/arch/event.hpp"

#endif
