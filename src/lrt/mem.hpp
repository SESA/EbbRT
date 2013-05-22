#ifndef EBBRT_LRT_MEM_HPP
#define EBBRT_LRT_MEM_HPP

#include <cstddef>
#include "lrt/event.hpp"

namespace ebbrt {
  namespace lrt {
    namespace mem {
      extern char* mem_start;
      /**
       * @brief Initial memory configuration 
       *
       * @param num_cores
       *
       * @return 
       */
      bool init(unsigned num_cores);
      /**
       * @brief Lrt memory allocator
       *
       * @param size 
       * @param loc
       *
       * @return 
       */
      void* malloc(size_t size, event::Location loc);
      /**
       * @brief Lrt memory alignment
       *
       * @param boundary
       * @param size
       * @param loc
       *
       * @return 
       */
      void* memalign(size_t boundary, size_t size, event::Location loc);
    }
  }
}

#endif
