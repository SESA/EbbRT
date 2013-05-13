#ifndef EBBRT_LRT_EVENT_HPP
#error "Don't include this file directly"
#endif

namespace ebbrt {
  namespace lrt {
    namespace event {
      bool init_arch(int num_cores);
      void init_cpu_arch() __attribute__((noreturn));
      inline unsigned get_num_cores() __attribute__((const));
      inline Location get_location() __attribute__((const));
    }
  }
}

#ifdef ARCH_X86_64
#include "lrt/arch/x86_64/event.hpp"
#else
#error "Unsupported Architecture"
#endif
