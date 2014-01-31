//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Trans.h>

#include <atomic>

#include <ebbrt/Cpu.h>
#include <ebbrt/CpuAsm.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EarlyPageAllocator.h>
#include <ebbrt/PageAllocator.h>
#include <ebbrt/VMem.h>

void ebbrt::trans::Init() {
  auto page = early_page_allocator::AllocatePage(1, Cpu::GetMyNode());
  vmem::TraversePageTable(
      vmem::page_table_root, kVMemStart, kVMemStart + pmem::kPageSize, 0, 4,
      [=](vmem::Pte& entry, uint64_t base_virt, size_t level) {
        kassert(!entry.Present());
        entry.Set(page.ToAddr() + (base_virt - kVMemStart), level > 0);
        std::atomic_thread_fence(std::memory_order_release);
        asm volatile("invlpg (%[addr])" : : [addr] "r"(base_virt) : "memory");
      },
      [](vmem::Pte& entry) {
        auto page = early_page_allocator::AllocatePage(1, Cpu::GetMyNode());
        auto page_addr = page.ToAddr();
        new (reinterpret_cast<void*>(page_addr)) vmem::Pte[512];
        entry.SetNormal(page_addr);
        return true;
      });
  std::memset(reinterpret_cast<void*>(page.ToAddr()), 0, pmem::kPageSize);
}

void ebbrt::trans::ApInit(size_t index) {
  auto pte_root = vmem::Pte(ReadCr3());
  auto idx = vmem::PtIndex(kVMemStart, 3);
  auto pt = reinterpret_cast<vmem::Pte*>(pte_root.Addr(false));
  pt[idx].Clear();

  auto nid = Cpu::GetByIndex(index)->nid();
  auto& p_allocator = PageAllocator::allocators[nid.val()];

  vmem::TraversePageTable(
      pte_root, kVMemStart, kVMemStart + pmem::kPageSize, 0, 4,
      [&](vmem::Pte& entry, uint64_t base_virt, size_t level) {
        kassert(!entry.Present());
        auto page = p_allocator.Alloc(0, nid);
        std::memset(reinterpret_cast<void*>(page.ToAddr()), 0, pmem::kPageSize);
        entry.Set(page.ToAddr() + (base_virt - kVMemStart), level > 0);
        std::atomic_thread_fence(std::memory_order_release);
        asm volatile("invlpg (%[addr])" : : [addr] "r"(base_virt) : "memory");
      },
      [&](vmem::Pte& entry) {
        auto page = p_allocator.Alloc(0, nid);
        auto page_addr = page.ToAddr();
        new (reinterpret_cast<void*>(page_addr)) vmem::Pte[512];
        entry.SetNormal(page_addr);
        return true;
      });
}
