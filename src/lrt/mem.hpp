#ifndef EBBRT_LRT_MEM_HPP
#define EBBRT_LRT_MEM_HPP

#include <cstddef>
#include "lrt/event.hpp"

namespace ebbrt {
  namespace lrt {
    /**
     * The lrt memory allocator.
     * This subsystem is only to be used at bringup. When the
     * ebbrt::memory_allocator is set up it should take over.
     * Memory allocated from here is never freed
     */
    namespace mem {
      /**
       * The start of memory to be allocated from.
       * This can be used very early on before this subsystem is
       * initialized to get memory
       */
      extern char* mem_start;
      /**
       * Initialize the memory subsystem
       * @param[in] num_cores Number of cores that will be initialized.
       */
      void init(unsigned num_cores);
      /**
       * Allocate memory
       *
       * @param[in] size The number of bytes to allocate
       * @param[in] loc Which core to allocate the memory from
       *
       * @return The allocated memory
       */
      void* malloc(size_t size, event::Location loc);
      /**
       * Allocate aligned memory
       *
       * @param[in] boundary The alignment of the allocated memory
       * @param[in] size The number of bytes to allocate
       * @param[in] loc Which core to allocate the memory from
       *
       * @return The allocated memory
       */
      void* memalign(size_t boundary, size_t size, event::Location loc);
    }
  }
}

#endif
