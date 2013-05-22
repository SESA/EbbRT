#include <cstdio>
#include <cstdint>

#include "lrt/boot.hpp"


namespace ebbrt {
  namespace lrt {
    namespace boot {
      void
      init_smp(unsigned num_cores)
      {
        std::printf("init smp\n");
        while(1)
          ;
      }
    }
  }
}
