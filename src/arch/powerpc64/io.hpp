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
#ifndef EBBRT_ARCH_IO_HPP
#error "Don't include this file directly"
#endif

#ifdef __linux__
#include <cassert>
#elif __ebbrt__
#include "lrt/bare/assert.hpp"
#endif

inline void
ebbrt::out8(uint8_t val, uint16_t port)
{
#ifdef __linux__
  assert(0);
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}

inline uint8_t
ebbrt::in8(uint16_t port)
{
#ifdef __linux__
  assert(0);
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
  return 0;
}

inline void
ebbrt::out16(uint16_t val, uint16_t port)
{
#ifdef __linux__
  assert(0);
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}

inline uint16_t
ebbrt::in16(uint16_t port)
{
#ifdef __linux__
  assert(0);
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
  return 0;
}

inline void
ebbrt::out32(uint32_t val, uint16_t port)
{
#ifdef __linux__
  assert(0);
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}

inline uint32_t
ebbrt::in32(uint16_t port)
{
#ifdef __linux__
  assert(0);
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
  return 0;
}
