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
#include <cstring>
#include <new>

#include "ebb/DistributedRoot.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "lrt/trans_impl.hpp"
#include "src/lrt/config.hpp"

#ifdef __ebbrt__
#include "lrt/bare/mem_impl.hpp"
#endif

using namespace ebbrt;

#ifdef __ebbrt__
ADD_EARLY_CONFIG_SYMBOL(MemoryAllocator,
			&ebbrt::SimpleMemoryAllocatorConstructRoot)
#endif

ebbrt::SimpleMemoryAllocator::SimpleMemoryAllocator(EbbId id, Location loc) :
MemoryAllocator{id}
{
#ifdef LRT_BARE
  lrt::mem::Region* r = &lrt::mem::regions[loc];
  start_ = r->start;
  current_ = r->current;
  end_ = r->end;
  r->start = r->end;
#endif
}

void*
ebbrt::SimpleMemoryAllocator::Malloc(size_t size)
{
  return Memalign(8, size);
}

void*
ebbrt::SimpleMemoryAllocator::Memalign(size_t boundary, size_t size)
{
#if __linux__
  assert(0);
  return nullptr;
#elif __ebbrt__
  char* p = current_;
  p = reinterpret_cast<char*>(((reinterpret_cast<uintptr_t>(p) +
                                boundary - 1) / boundary) * boundary);

  if ((p + size) > end_) {
    return nullptr;
  }
  current_ = p + size;
  return p;
#endif
}

void
ebbrt::SimpleMemoryAllocator::Free(void* p)
{
#if __linux__
  assert(0);
#elif __ebbrt__
#endif
}

void*
ebbrt::SimpleMemoryAllocator::Realloc(void* ptr, size_t size)
{
  Free(ptr);
  return Malloc(size);
}

void*
ebbrt::SimpleMemoryAllocator::Calloc(size_t num, size_t size)
{
  size_t total = num * size;
  void* ptr = malloc(total);
  return std::memset(ptr, 0, total);
}

void*
ebbrt::SimpleMemoryAllocator::operator new(size_t size)
{
#ifdef __linux__
  return std::malloc(size);
#elif __ebbrt__
  return lrt::mem::malloc(size, get_location());
#endif
}

void
ebbrt::SimpleMemoryAllocator::operator delete(void* p)
{
}

ebbrt::EbbRoot* ebbrt::SimpleMemoryAllocatorConstructRoot()
{
  static DistributedRoot<SimpleMemoryAllocator> root;
  return &root;
}
