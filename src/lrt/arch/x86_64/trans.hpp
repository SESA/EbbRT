#ifndef EBBRT_LRT_TRANS_HPP
#error "Don't include this file directly'"
#endif

namespace ebbrt {
  namespace lrt {
    namespace trans {
      void* const LOCAL_MEM_VIRT = reinterpret_cast<void*>(0xFFFFFFFF00000000);
    }
  }
}
