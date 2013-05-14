#include <cstring>

#include <algorithm>
#include <new>

#include "arch/x86_64/mmu.hpp"
#include "lrt/event.hpp"
#include "lrt/mem.hpp"
#include "lrt/trans_impl.hpp"

void
ebbrt::lrt::trans::init_cpu_arch()
{
  Pml4Entry* my_pml4 = new
    (mem::memalign(PML4_ALIGN, PML4_SIZE, event::get_location()))
    Pml4Entry[PML4_NUM_ENTS];

  PdptEntry* trans_pdpt = new
    (mem::memalign(PDPT_ALIGN, PDPT_SIZE, event::get_location()))
    PdptEntry[PDPT_NUM_ENTS];

  Pd2mEntry* trans_pdir = new
    (mem::memalign(PDIR_ALIGN, PDIR_SIZE, event::get_location()))
    Pd2mEntry[PDIR_NUM_ENTS];

  void* local_mem_phys = mem::memalign(1 << 21,
                                       LTABLE_SIZE,
                                       event::get_location());

  phys_local_entries[event::get_location()] =
    static_cast<LocalEntry*>(local_mem_phys);

  Pml4Entry* pml4 = getPml4();
  PdptEntry* existing_pdpt = reinterpret_cast<PdptEntry*>
    (pml4[pml4_index(LOCAL_MEM_VIRT)].base << 12);

  std::copy(pml4, pml4 + PML4_NUM_ENTS, my_pml4);
  std::copy(existing_pdpt, existing_pdpt + PDPT_NUM_ENTS, trans_pdpt);
  std::memset(trans_pdir, 0, PDIR_SIZE);

  trans_pdir[pdir_index(LOCAL_MEM_VIRT)].present = 1;
  trans_pdir[pdir_index(LOCAL_MEM_VIRT)].rw = 1;
  trans_pdir[pdir_index(LOCAL_MEM_VIRT)].ps = 1;
  trans_pdir[pdir_index(LOCAL_MEM_VIRT)].base =
    reinterpret_cast<uint64_t>(local_mem_phys) >> 21;

  trans_pdpt[pdpt_index(LOCAL_MEM_VIRT)].present = 1;
  trans_pdpt[pdpt_index(LOCAL_MEM_VIRT)].rw = 1;
  trans_pdpt[pdpt_index(LOCAL_MEM_VIRT)].base =
    reinterpret_cast<uint64_t>(trans_pdir) >> 12;

  my_pml4[pml4_index(LOCAL_MEM_VIRT)].present = 1;
  my_pml4[pml4_index(LOCAL_MEM_VIRT)].rw = 1;
  my_pml4[pml4_index(LOCAL_MEM_VIRT)].base =
    reinterpret_cast<uint64_t>(trans_pdpt) >> 12;

  set_pml4(my_pml4);
}
