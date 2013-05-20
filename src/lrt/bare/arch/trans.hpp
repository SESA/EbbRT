#ifndef EBBRT_LRT_TRANS_HPP
#error "Don't include this file directly'"
#endif

namespace ebbrt {
  namespace lrt {
    namespace trans {
      void init_cpu_arch();
    }
  }
}

#ifdef ARCH_X86_64
#include "lrt/bare/arch/x86_64/trans.hpp"
#else
#error "Unsupported Architecture"
#endif
