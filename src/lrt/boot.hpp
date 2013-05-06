#ifndef EBBRT_LRT_BOOT_HPP
#define EBBRT_LRT_BOOT_HPP

#include "lrt/arch/boot.hpp"

namespace ebbrt {
  namespace lrt {
    namespace boot {
      void init() __attribute__((noreturn));
    }
  }
}

#endif
