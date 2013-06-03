#include <cstdio>
#include <cstdint>

#include "lrt/boot.hpp"
#include "lrt/event.hpp"

namespace ebbrt {
  namespace lrt {
    namespace boot {
      void
      init_smp(unsigned num_cores)
      {
        /* no SMP to init on ulnx */
        event::init_cpu();
      }
    }
  }
}
