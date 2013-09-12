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
#ifndef EBBRT_ARCH_INET_HPP
#define EBBRT_ARCH_INET_HPP

#include <cstdint>

namespace ebbrt {
  inline uint16_t htons(uint16_t hostshort);
  inline uint16_t ntohs(uint16_t netshort);
}

#ifdef ARCH_X86_64
#include "arch/x86_64/inet.hpp"
#else
#error "Unsupported Architecture"
#endif

#endif
