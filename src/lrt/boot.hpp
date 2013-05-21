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
      /** @brief Initialize core system functionality. This should be
       * initiated before other cores are activated */
      void init() __attribute__((noreturn));
      /**
       * @brief Wake up secondary cores
       *
       * @param num_core 
       */
      void init_smp(unsigned num_cores) __attribute__((noreturn));
      /**
       * @brief Individual core initiation
       */
      void init_cpu();
    }
  }
}

#endif
