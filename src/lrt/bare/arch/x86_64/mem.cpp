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
#include "arch/x86_64/multiboot.hpp"
#include "lrt/bare/assert.hpp"
#include "lrt/bare/boot.hpp"
#include "lrt/bare/mem_impl.hpp"

void
ebbrt::lrt::mem::init(unsigned num_cores)
{
  /**@brief Partition available address space into core-specific regions */
  LRT_ASSERT(boot::multiboot_information->has_mem);

  /* adjust mem_start to account for bootloaded modules */
  if(boot::multiboot_information->modules_count > 0){
    auto addr_32 = static_cast<uintptr_t>(lrt::boot::multiboot_information->modules_address);
    auto addr = reinterpret_cast<uint32_t *>(addr_32);
    uint32_t mod_val = *(addr + 1 + ((boot::multiboot_information->modules_count - 1) * 4));
    mem_start = reinterpret_cast<char *>(mod_val);
  }
  
  /* regions array stored at start of free memory */
  regions = reinterpret_cast<Region*>(mem_start);
  char* ptr = mem_start + (num_cores * sizeof(Region));

  /* remaining memory is calculated and divided equally between cores */
  uint64_t num_bytes = static_cast<uint64_t>
    (boot::multiboot_information->memory_higher) << 10;

  /* 1MB being the start of the kernel */
  num_bytes -= reinterpret_cast<uint64_t>(mem_start) - 0x100000;
  for (unsigned i = 0; i < num_cores; ++i) {
    regions[i].start = regions[i].current = ptr;
    ptr += num_bytes / num_cores;
    regions[i].end = ptr;
  }
}
