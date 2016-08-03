//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/VMem.h>

#include <atomic>
#include <cinttypes>
#include <cstdint>
#include <cstring>

#include <ebbrt/Align.h>
#include <ebbrt/CpuAsm.h>
#include <ebbrt/Debug.h>
#include <ebbrt/E820.h>
#include <ebbrt/EarlyPageAllocator.h>
#include <ebbrt/PageAllocator.h>

namespace {
ebbrt::ExplicitlyConstructed<ebbrt::vmem::Pte> page_table_root;
}

void ebbrt::vmem::Init() { page_table_root.construct(); }

void ebbrt::vmem::EarlyMapMemory(uint64_t addr, uint64_t length) {
  auto aligned_addr = align::Down(addr, pmem::kPageSize);
  auto aligned_length =
      align::Up(length + (addr - aligned_addr), pmem::kPageSize);

  TraversePageTable(
      GetPageTableRoot(), aligned_addr, aligned_addr + aligned_length, 0, 4,
      [=](Pte& entry, uint64_t base_virt, size_t level) {
        if (entry.Present()) {
          kassert(entry.Addr(level > 0) == base_virt);
          return;
        }
        entry.Set(base_virt, level > 0);
        std::atomic_thread_fence(std::memory_order_release);
        asm volatile("invlpg (%[addr])" : : [addr] "r"(base_virt) : "memory");
      },
      [=](Pte& entry) {
        auto page = early_page_allocator::AllocatePage();
        auto page_addr = page.ToAddr();
        new (reinterpret_cast<void*>(page_addr)) Pte[512];

        entry.SetNormal(page_addr);
        return true;
      });
}

void ebbrt::vmem::EarlyUnmapMemory(uint64_t addr, uint64_t length) {
  auto aligned_addr = align::Down(addr, pmem::kPageSize);
  auto aligned_length =
      align::Up(length + (addr - aligned_addr), pmem::kPageSize);

  TraversePageTable(
      GetPageTableRoot(), aligned_addr, aligned_addr + aligned_length, 0, 4,
      [=](Pte& entry, uint64_t base_virt, size_t level) {
        kassert(entry.Present());
        entry.SetPresent(false);
        std::atomic_thread_fence(std::memory_order_release);
        asm volatile("invlpg (%[addr])" : : [addr] "r"(base_virt) : "memory");
      },
      [=](Pte& entry) {
        kprintf("Asked to unmap memory that wasn't mapped!\n");
        kabort();
        return false;
      });
}

void ebbrt::vmem::MapMemory(Pfn vfn, Pfn pfn, uint64_t length) {
  auto pte_root = Pte(ReadCr3());
  auto vaddr = vfn.ToAddr();
  TraversePageTable(pte_root, vaddr, vaddr + length, 0, 4,
                    [=](Pte& entry, uint64_t base_virt, size_t level) {
                      kassert(!entry.Present());
                      entry.Set(pfn.ToAddr() + (base_virt - vaddr), level > 0);
                      std::atomic_thread_fence(std::memory_order_release);
                    },
                    [](Pte& entry) {
                      auto page = page_allocator->Alloc();
                      kbugon(page == Pfn::None());
                      auto page_addr = page.ToAddr();
                      new (reinterpret_cast<void*>(page_addr)) Pte[512];
                      entry.SetNormal(page_addr);
                      return true;
                    });
}

// traverses per core page table and backs vaddr with physical pages
// in pfn
void ebbrt::vmem::MapMemoryLarge(uintptr_t vaddr, Pfn pfn, uint64_t length) {
  auto pte_root = Pte(ReadCr3());
  TraversePageTable(
      pte_root, vaddr, vaddr + length, 0, 4,
      [=](Pte& entry, uint64_t base_virt, size_t level) {
        kassert(!entry.Present());
        entry.Set(pfn.ToAddr() + (base_virt - vaddr), level > 0);
        std::atomic_thread_fence(std::memory_order_release);
      },
      [](Pte& entry) {
        auto page = page_allocator->Alloc();
        kbugon(page == Pfn::None());
        auto page_addr = page.ToAddr();
        new (reinterpret_cast<void*>(page_addr)) Pte[512];
        entry.SetNormal(page_addr);
        return true;
      });
}

void ebbrt::vmem::EnableRuntimePageTable() {
  asm volatile("mov %[page_table], %%cr3"
               :
               : [page_table] "r"(GetPageTableRoot()));
}

void ebbrt::vmem::ApInit(size_t index) {
  EnableRuntimePageTable();
  Pte ap_pte_root;
  auto nid = Cpu::GetByIndex(index)->nid();
  auto& p_allocator = (*PageAllocator::allocators)[nid.val()];
  auto page = p_allocator.Alloc(0, nid);
  kbugon(page == Pfn::None(),
         "Failed to allocate page for initial page tables\n");
  auto page_addr = page.ToAddr();
  std::memcpy(reinterpret_cast<void*>(page_addr),
              reinterpret_cast<void*>(GetPageTableRoot().Addr(false)), 4096);
  ap_pte_root.SetNormal(page_addr);

  asm volatile("mov %[page_table], %%cr3" : : [page_table] "r"(ap_pte_root));
}

ebbrt::vmem::Pte& ebbrt::vmem::GetPageTableRoot() { return *page_table_root; }
