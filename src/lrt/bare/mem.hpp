/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef EBBRT_LRT_BARE_MEM_HPP
#define EBBRT_LRT_BARE_MEM_HPP

#include <cstddef>
#include "lrt/Location.hpp"

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
