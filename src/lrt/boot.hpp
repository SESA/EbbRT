#ifndef EBBRT_LRT_BOOT_HPP
#define EBBRT_LRT_BOOT_HPP

#include "lrt/arch/boot.hpp"
#include "lrt/trans.hpp"

namespace ebbrt {
  namespace lrt {
    namespace boot {
      void init() __attribute__((noreturn));
      void init_smp(unsigned num_cores) __attribute__((noreturn));
      void init_cpu();
    }
  }
}

#endif
