#ifndef EBBRT_LRT_MEM_HPP
#define EBBRT_LRT_MEM_HPP

#include "lrt/event.hpp"

namespace ebbrt {
  namespace lrt {
    namespace mem {
      extern char* mem_start;
      bool init(unsigned num_cores);
      void* malloc(size_t size, event::Location loc);
      void* memalign(size_t boundary, size_t size, event::Location loc);
    }
  }
}

#endif
