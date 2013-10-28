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
#ifndef EBBRT_LRT_BARE_BOOT_HPP
#error "Don't include this file directly"
#endif

#include <atomic>
#include <cstdint>
#include <string>

#include "arch/x86_64/multiboot.hpp"

namespace ebbrt {
  namespace lrt {
    namespace boot {
      extern uint32_t smp_lock;
      extern const MultibootInformation* multiboot_information;
      extern void* fdt; // flattened device tree
    }
  }
}
