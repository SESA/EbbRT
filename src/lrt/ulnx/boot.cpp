#include <cstdio>
#include <cstdint>

#include "lrt/boot.hpp"


namespace ebbrt {
  namespace lrt {
    namespace boot {
      void
      init_smp(unsigned num_cores)
      {
        /* no SMP to init on ulnx */
        init_cpu();
        /*FIXME: this spin to appease compiler */
        while(1)
          ;
      }
    }
  }
}
