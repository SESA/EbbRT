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
#ifndef EBBRT_EBB_MEMORYALLOCATOR_SIMPLEMEMORYALLOCATOR_HPP
#define EBBRT_EBB_MEMORYALLOCATOR_SIMPLEMEMORYALLOCATOR_HPP

#include <unordered_map>

#include "ebb/MemoryAllocator/MemoryAllocator.hpp"


namespace ebbrt {
  class SimpleMemoryAllocator : public MemoryAllocator {
  public:
    SimpleMemoryAllocator(EbbId id, Location loc = get_location());
    void* Malloc(size_t size) override;
    void* Memalign(size_t boundary, size_t size) override;
    void Free(void* ptr) override;
    void* Realloc(void* ptr, size_t size) override;
    void* Calloc(size_t num, size_t size) override;
    static void* operator new(size_t size);
    static void operator delete(void* p);
  private:
    char* start_;
    char* current_;
    char* end_;
  };

  EbbRoot* SimpleMemoryAllocatorConstructRoot();
}

#endif
