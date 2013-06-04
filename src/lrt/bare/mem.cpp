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
#include "lrt/bare/mem_impl.hpp"

extern char kend[];
char* ebbrt::lrt::mem::mem_start = kend;

ebbrt::lrt::mem::Region* ebbrt::lrt::mem::regions;

void*
ebbrt::lrt::mem::malloc(size_t size, event::Location loc)
{
  return memalign(8, size, loc);
}

void*
ebbrt::lrt::mem::memalign(size_t boundary, size_t size, event::Location loc)
{
  Region* r = &regions[loc];
  char* p = r->current;
  p = reinterpret_cast<char*>(((reinterpret_cast<uintptr_t>(p) + boundary - 1)
                               / boundary) * boundary);
  if ((p + size) > r->end) {
    return nullptr;
  }
  r->current = p + size;
  return p;
}
