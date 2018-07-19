//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_GENERALPURPOSEALLOCATOR_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_GENERALPURPOSEALLOCATOR_H_

#include <array>

#include "../CacheAligned.h"
#include "CpuAsm.h"
#include "Debug.h"
#include "SlabAllocator.h"
#include "Trans.h"
#include "VMemAllocator.h"

namespace ebbrt {

// handler used in Pci.cc code to handle faults on multicores when mapping
// device
class MulticorePciFaultHandler : public ebbrt::VMemAllocator::PageFaultHandler {
  ebbrt::Pfn vpage_;
  ebbrt::Pfn ppage_;
  size_t size_;

 public:
  void SetMap(ebbrt::Pfn va, ebbrt::Pfn pa, size_t s) {
    vpage_ = va;
    ppage_ = pa;
    size_ = s;
  }

  void HandleFault(ebbrt::idt::ExceptionFrame* ef,
                   uintptr_t faulted_address) override {
    ebbrt::vmem::MapMemory(vpage_, ppage_, size_);
  }
};

// page fault handler for mapping in physical pages
// to virtual pages on all cores
class LargeRegionFaultHandler : public ebbrt::VMemAllocator::PageFaultHandler {
  std::vector<ebbrt::Pfn> vecPfns;
  uintptr_t sAddr;  // start addr of region

 public:
  void SetAddr(uintptr_t s) { sAddr = s; }
  void SetVec(std::vector<ebbrt::Pfn> m) { vecPfns = std::move(m); }

  // given faulted address, calculates virtual page frame and finds
  // corresponding physical page frame in tmap table, then calls
  // MapMemory to do actual mapping
  void HandleFault(ebbrt::idt::ExceptionFrame* ef,
                   uintptr_t faulted_address) override {
    auto vpage = ebbrt::Pfn::LDown(faulted_address);

    // calculate number of 2Mb pages from sAddr to
    // use as index to Pfns
    auto mAddr = vpage.ToLAddr();
    kassert(mAddr >= sAddr);
    auto index = (mAddr - sAddr) / pmem::kLargePageSize;
    kassert(index < vecPfns.size());
    ebbrt::vmem::MapMemoryLarge(mAddr, vecPfns[index], pmem::kLargePageSize);
  }
};

template <size_t... sizes_in>
class GeneralPurposeAllocator : public CacheAligned {
 public:
  static void Init() {
    rep_allocator =
        new SlabAllocatorRoot(sizeof(GeneralPurposeAllocator<sizes_in...>),
                              alignof(GeneralPurposeAllocator<sizes_in...>));

    Construct<0, sizes_in...> c;
    c(allocator_roots);
  }

  static GeneralPurposeAllocator<sizes_in...>& HandleFault(EbbId id) {
    if (reps[Cpu::GetMine()] == nullptr) {
      reps[Cpu::GetMine()] = new GeneralPurposeAllocator<sizes_in...>();
    }

    auto& allocator = *reps[Cpu::GetMine()];
    EbbRef<GeneralPurposeAllocator<sizes_in...>>::CacheRef(id, allocator);
    return allocator;
  }

  GeneralPurposeAllocator() {
    for (size_t i = 0; i < allocators_.size(); ++i) {
      allocators_[i] = &allocator_roots[i]->GetCpuAllocator();
    }
  }

  void* operator new(size_t size) {
    auto& allocator = rep_allocator->GetCpuAllocator();
    auto ret = allocator.Alloc();

    if (ret == nullptr)
      throw std::bad_alloc();

    return ret;
  }

  void operator delete(void* p) { EBBRT_UNIMPLEMENTED(); }

  void* Alloc(size_t size) {
    Indexer<0, sizes_in...> i;
    auto index = i(size);
    if (likely(index != -1)) {
      auto ret = allocators_[index]->Alloc();
      kbugon(ret == nullptr,
             "Failed to allocate from this NUMA node, should try others\n");
      return ret;
    }
    const constexpr size_t large_page_size = 2 * 1024 * 1024;
    const constexpr size_t large_page_order = 9;
    auto sz = align::Up(size, large_page_size);
    auto npages = sz / pmem::kPageSize;
    auto pages_per_large_page = large_page_size / pmem::kPageSize;

    auto pf = std::make_unique<LargeRegionFaultHandler>();
    auto& ref = *pf;  // keep reference for update later
    std::vector<ebbrt::Pfn> vecPfns;

    // Need to allocate a virtual region
    auto vfn =
        vmem_allocator->Alloc(npages, pages_per_large_page, std::move(pf));
    kbugon(vfn == Pfn::None(), "Failed to allocated virtual region\n");
    auto pte_root = vmem::Pte(ReadCr3());
    auto vaddr = vfn.ToAddr();

    vmem::TraversePageTable(
        pte_root, vaddr, vaddr + sz, 0, 4,
        [&vecPfns](vmem::Pte& entry, uint64_t base_virt, size_t level) {
          kassert(!entry.Present() && level == 1);
          auto pfn = page_allocator->Alloc(large_page_order);
          kbugon(pfn == Pfn::None(),
                 "Failed to allocate page in gp allocator\n");
          vecPfns.emplace_back(pfn);  // store pfn
          entry.SetLarge(pfn.ToAddr());
          std::atomic_thread_fence(std::memory_order_release);
        },
        [](vmem::Pte& entry) {
          auto page = page_allocator->Alloc();
          kbugon(page == Pfn::None(),
                 "Failed to allocate page in gp allocator\n");
          auto page_addr = page.ToAddr();
          new (reinterpret_cast<void*>(page_addr)) vmem::Pte[512];
          entry.SetNormal(page_addr);
          return true;
        });

    // updates page fault handler for large memory regions
    ref.SetAddr(vaddr);
    ref.SetVec(std::move(vecPfns));

    return reinterpret_cast<void*>(vaddr);
  }

  void* Alloc(size_t size, size_t alignment) {
    Indexer<0, sizes_in...> i;
    auto index = i(size);
    if (likely(index != -1)) {
      auto ret = allocators_[index]->Alloc();
      kbugon(ret == nullptr,
             "Failed to allocate from this NUMA node, should try others\n");
      return ret;
    }
    const constexpr size_t large_page_size = 2 * 1024 * 1024;
    const constexpr size_t large_page_order = 9;
    auto sz = align::Up(size, large_page_size);
    auto npages = sz / pmem::kPageSize;
    auto align = align::Up(alignment, large_page_size);
    auto align_pages = align / pmem::kPageSize;
    // Need to allocate a virtual region
    auto vfn = vmem_allocator->Alloc(npages, align_pages);
    kbugon(vfn == Pfn::None(), "Failed to allocated virtual region\n");
    auto pte_root = vmem::Pte(ReadCr3());
    auto vaddr = vfn.ToAddr();
    vmem::TraversePageTable(
        pte_root, vaddr, vaddr + sz, 0, 4,
        [](vmem::Pte& entry, uint64_t base_virt, size_t level) {
          kassert(!entry.Present() && level == 1);
          auto pfn = page_allocator->Alloc(large_page_order);
          kbugon(pfn == Pfn::None(),
                 "Failed to allocate page in gp allocator\n");
          entry.SetLarge(pfn.ToAddr());
          std::atomic_thread_fence(std::memory_order_release);
        },
        [](vmem::Pte& entry) {
          auto page = page_allocator->Alloc();
          kbugon(page == Pfn::None(),
                 "Failed to allocate page in gp allocator\n");
          auto page_addr = page.ToAddr();
          new (reinterpret_cast<void*>(page_addr)) vmem::Pte[512];
          entry.SetNormal(page_addr);
          return true;
        });
    return reinterpret_cast<void*>(vaddr);
  }

  void* AllocNid(size_t size, Nid nid = Cpu::GetMyNode()) {
    Indexer<0, sizes_in...> i;
    auto index = i(size);
    kbugon(index == -1, "Attempt to allocate %u bytes not supported\n", size);
    auto ret = allocators_[index]->AllocNid(nid);
    kbugon(ret == nullptr,
           "Failed to allocate from this NUMA node, should try others\n");
    return ret;
  }

  void Free(void* p) {
    if (p == nullptr)
      return;
    if (reinterpret_cast<uintptr_t>(p) > 0xFFFF800000000000) {
      kprintf("UNIMPLEMENTED: Free Large memory region\n");
      return;
    }
    auto page = mem_map::AddrToPage(p);
    kassert(page != nullptr);

    auto& allocator = page->data.slab_data.cache->root_.GetCpuAllocator();
    allocator.Free(p);
  }

 private:
  template <size_t index, size_t... tail> struct Construct {
    void
    operator()(std::array<SlabAllocatorRoot*, sizeof...(sizes_in)>& roots) {}
  };

  template <size_t index, size_t head, size_t... tail>
  struct Construct<index, head, tail...> {
    static_assert(head <= SlabAllocator::kMaxSlabSize,
                  "gp allocator instantiation failed, "
                  "request for slab allocator is too "
                  "large");
    void
    operator()(std::array<SlabAllocatorRoot*, sizeof...(sizes_in)>& roots) {
      roots[index] = new SlabAllocatorRoot(head);
      Construct<index + 1, tail...> next;
      next(roots);
    }
  };

  template <size_t index, size_t... tail> struct Indexer {
    ssize_t operator()(size_t size) { return -1; }
  };

  template <size_t index, size_t head, size_t... tail>
  struct Indexer<index, head, tail...> {
    ssize_t operator()(size_t size) {
      if (size <= head) {
        return index;
      } else {
        Indexer<index + 1, tail...> i;
        return i(size);
      }
    }
  };

  static std::array<SlabAllocatorRoot*, sizeof...(sizes_in)> allocator_roots;
  static std::array<GeneralPurposeAllocator<sizes_in...>*, Cpu::kMaxCpus> reps;
  static SlabAllocatorRoot* rep_allocator;
  std::array<SlabAllocator*, sizeof...(sizes_in)> allocators_;
};

template <size_t... sizes_in>
std::array<SlabAllocatorRoot*, sizeof...(sizes_in)>
    GeneralPurposeAllocator<sizes_in...>::allocator_roots;

template <size_t... sizes_in>
std::array<GeneralPurposeAllocator<sizes_in...>*, Cpu::kMaxCpus>
    GeneralPurposeAllocator<sizes_in...>::reps;

template <size_t... sizes_in>
SlabAllocatorRoot* GeneralPurposeAllocator<sizes_in...>::rep_allocator;

// roughly equivalent to the breakdown Linux uses, may need tuning
typedef GeneralPurposeAllocator<
    8, 16, 32, 64, 96, 128, 192, 256, 512, 1024, 2 * 1024, 4 * 1024, 8 * 1024,
    16 * 1024, 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024, 512 * 1024,
    1024 * 1024, 2 * 1024 * 1024, 4 * 1024 * 1024, 8 * 1024 * 1024>
    GeneralPurposeAllocatorType;

constexpr auto gp_allocator =
    EbbRef<GeneralPurposeAllocatorType>(kGpAllocatorId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_GENERALPURPOSEALLOCATOR_H_
