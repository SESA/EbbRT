#ifndef EBBRT_LRT_MEM_IMPL_HPP
#define EBBRT_LRT_MEM_IMPL_HPP

#include "lrt/mem.hpp"

namespace ebbrt {
  namespace lrt {
    namespace mem {
      /**
       * @brief Memory region for per-core allocation 
       */
      class Region {
      public:
        char* start;
        char* current;
        char* end;
      };
      extern Region* regions;
    }
  }
}
#endif
