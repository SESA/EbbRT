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

#include <algorithm>
#include <new>

#include "arch/x86_64/mmu.hpp"
#include "lrt/event.hpp"
#include "lrt/bare/mem.hpp"
#include "lrt/trans_impl.hpp"

/**
 * @brief When cores wake up then latch onto the virtual memory paging
 * structure set up by the primary core at boot. In this function we strive to
 * associate an array allocated per-core with a single virtual address. To
 * achieve this, we must set up each core with their own VM structure.
 */
void
ebbrt::lrt::trans::init_cpu_arch()
{
  /* allocate new paging structures for this core */
  Pml4Entry* my_pml4 = new
    (mem::memalign(PML4_ALIGN, PML4_SIZE, event::get_location()))
    Pml4Entry[PML4_NUM_ENTS];

  PdptEntry* trans_pdpt = new
    (mem::memalign(PDPT_ALIGN, PDPT_SIZE, event::get_location()))
    PdptEntry[PDPT_NUM_ENTS];

  Pd2mEntry* trans_pdir = new
    (mem::memalign(PDIR_ALIGN, PDIR_SIZE, event::get_location()))
    Pd2mEntry[PDIR_NUM_ENTS];

  /* physical address of local memory */
  void* local_mem_phys = mem::memalign(1 << 21,
                                       LTABLE_SIZE,
                                       event::get_location());

  /* store table physical starting address */
  phys_local_entries[event::get_location()] =
    static_cast<LocalEntry*>(local_mem_phys);

  /* grab and copy existing vm state into new structure */
  Pml4Entry* pml4 = getPml4();
  PdptEntry* existing_pdpt = reinterpret_cast<PdptEntry*>
    (pml4[pml4_index(LOCAL_MEM_VIRT_BEGIN)].base << 12);
  std::copy(pml4, pml4 + PML4_NUM_ENTS, my_pml4);
  std::copy(existing_pdpt, existing_pdpt + PDPT_NUM_ENTS, trans_pdpt);
  /* mark all pages as invalid */
  std::memset(trans_pdir, 0, PDIR_SIZE);

  /* set the entries we want as present*/
  trans_pdir[pdir_index(LOCAL_MEM_VIRT_BEGIN)].present = 1;
  trans_pdir[pdir_index(LOCAL_MEM_VIRT_BEGIN)].rw = 1;
  trans_pdir[pdir_index(LOCAL_MEM_VIRT_BEGIN)].ps = 1;
  trans_pdir[pdir_index(LOCAL_MEM_VIRT_BEGIN)].base =
    reinterpret_cast<uint64_t>(local_mem_phys) >> 21;

  trans_pdpt[pdpt_index(LOCAL_MEM_VIRT_BEGIN)].present = 1;
  trans_pdpt[pdpt_index(LOCAL_MEM_VIRT_BEGIN)].rw = 1;
  trans_pdpt[pdpt_index(LOCAL_MEM_VIRT_BEGIN)].base =
    reinterpret_cast<uint64_t>(trans_pdir) >> 12;

  my_pml4[pml4_index(LOCAL_MEM_VIRT_BEGIN)].present = 1;
  my_pml4[pml4_index(LOCAL_MEM_VIRT_BEGIN)].rw = 1;
  my_pml4[pml4_index(LOCAL_MEM_VIRT_BEGIN)].base =
    reinterpret_cast<uint64_t>(trans_pdpt) >> 12;

  /* update cores pointer to new paging structures */
  set_pml4(my_pml4);
}
