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
#include "lrt/bare/mem_impl.hpp"
#include "lrt/trans_impl.hpp"

using namespace ebbrt;

ebbrt::SimpleMemoryAllocator::SimpleMemoryAllocator(Location loc)
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
ebbrt::SimpleMemoryAllocator::malloc(size_t size)
{
  return memalign(8, size);
}

void*
ebbrt::SimpleMemoryAllocator::memalign(size_t boundary, size_t size) {
  char* p = current_;
  p = reinterpret_cast<char*>(((reinterpret_cast<uintptr_t>(p) +
                                boundary - 1) / boundary) * boundary);

  if ((p + size) > end_) {
    return nullptr;
  }
  current_ = p + size;
  return p;
}

void
ebbrt::SimpleMemoryAllocator::free(void* p)
{
}

void*
ebbrt::SimpleMemoryAllocator::realloc(void* ptr, size_t size)
{
  free(ptr);
  return malloc(size);
}

void*
ebbrt::SimpleMemoryAllocator::calloc(size_t num, size_t size)
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
