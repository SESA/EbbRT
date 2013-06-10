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
#ifndef EBBRT_EBB_MEMORYALLOCATOR_MEMORYALLOCATOR_HPP
#define EBBRT_EBB_MEMORYALLOCATOR_MEMORYALLOCATOR_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class MemoryAllocator : public EbbRep {
  public:
    virtual void* Malloc(size_t size) = 0;
    virtual void* Memalign(size_t boundary, size_t size) = 0;
    virtual void Free(void* ptr) = 0;
    virtual void* Realloc(void* ptr, size_t size) = 0;
    virtual void* Calloc(size_t num, size_t size) = 0;
    virtual ~MemoryAllocator() {}
  };
  constexpr EbbRef<MemoryAllocator> memory_allocator =
    EbbRef<MemoryAllocator>(static_cast<EbbId>(1));
}

#endif
