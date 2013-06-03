#ifndef EBBRT_LRT_BOOT_HPP
#define EBBRT_LRT_BOOT_HPP

#ifdef LRT_ULNX
/*space holder*/
#elif LRT_BARE
#include <src/lrt/bare/boot.hpp>
#endif

#include "lrt/trans.hpp"

namespace ebbrt {
  namespace lrt {
    namespace boot {
      /**
       *  Initialize LRT.
       *  This is done once, before core-specific initialization.
      */
      void init() __attribute__((noreturn));
      /**
       * Start other cores then do per core initialization.
       * @param[in] num_cores The total number of cores to start
       */
      void init_smp(unsigned num_cores) __attribute__((noreturn));
      /**
       * Per core initialization.
       */
      void init_cpu();
    }
  }
}

#endif
