#ifndef EBBRT_LRT_ULNX_TRANS_HPP
#define EBBRT_LRT_ULNX_TRANS_HPP

#include <cstdlib>
namespace ebbrt {
  namespace lrt {
    namespace trans {
      void* const LOCAL_MEM_VIRT = std::malloc(1<<21);
    }
  }
}

#endif
