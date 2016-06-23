//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/PageAllocator.h>

#include <mutex>

#include <boost/container/static_vector.hpp>

#include <ebbrt/Align.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EarlyPageAllocator.h>
#include <ebbrt/MemMap.h>

ebbrt::ExplicitlyConstructed<boost::container::static_vector<
    ebbrt::PageAllocator, ebbrt::numa::kMaxNodes>>
    ebbrt::PageAllocator::allocators;

void ebbrt::PageAllocator::Init() {
  allocators.construct();
  for (unsigned i = 0; i < numa::nodes->size(); ++i) {
    allocators->emplace_back(Nid(i));
  }

  early_page_allocator::ReleaseFreePages([](Pfn start, Pfn end, Nid nid) {
    kassert(end - start != 0);
    // compute largest power of two that fits in the range
    auto largest_pow2 =
        1 << (sizeof(unsigned int) * 8 - __builtin_clz(end - start) - 1);
    // find the divide at which point the pages decrease in size
    auto divide = align::Up(start.val(), largest_pow2);
    auto pfn = start;
    // Add all the pages up to the divide
    while (pfn.val() != divide) {
      kassert(pfn.val() < divide);
      size_t order = __builtin_ctz(pfn.val());
      order = order > kMaxOrder ? kMaxOrder : order;
      EarlyFreePage(pfn, order, nid);
      pfn += 1 << order;
    }
    // now add the rest
    pfn = Pfn(divide);
    while (pfn != end) {
      kassert(pfn < end);
      auto order = sizeof(unsigned int) * 8 - __builtin_clz(end - pfn) - 1;
      order = order > kMaxOrder ? kMaxOrder : order;
      EarlyFreePage(pfn, order, nid);
      pfn += 1 << order;
    }
  });
#ifdef PAGE_CHECKER
  for (unsigned i = 0; i < numa::nodes->size(); ++i) {
    kassert((*allocators)[i].Validate());
  }
#endif
}

void ebbrt::PageAllocator::EarlyFreePage(Pfn start, size_t order, Nid nid) {
  kassert(order <= kMaxOrder);
  auto entry = PfnToFreePage(start);
  (*allocators)[nid.val()].free_page_lists[order].push_front(*entry);
  auto page = mem_map::PfnToPage(start);
  kassert(page != nullptr);
  page->usage = mem_map::Page::Usage::kPageAllocator;
  page->data.order = order;
}

ebbrt::PageAllocator& ebbrt::PageAllocator::HandleFault(EbbId id) {
  kassert(id == kPageAllocatorId);
  auto& allocator = (*allocators)[Cpu::GetMyNode().val()];
  EbbRef<PageAllocator>::CacheRef(id, allocator);
  return allocator;
}

ebbrt::PageAllocator::PageAllocator(Nid nid) : nid_(nid) {}

ebbrt::Pfn ebbrt::PageAllocator::AllocLocal(size_t order, uint64_t max_addr) {

  // cause a stack allocator page fault first, to
  // avoid deadlocking when calling malloc and
  // stack faulting at the same time
  int tmp;
  asm volatile("movl -4096(%%rsp), %0;" : "=r"(tmp) : :);

  std::lock_guard<SpinLock> lock(lock_);

  FreePage* fp = nullptr;
  auto this_order = order;
  while (this_order <= kMaxOrder) {
    auto& list = free_page_lists[this_order];
    auto it =
        std::find_if(list.begin(), list.end(), [max_addr](const FreePage& p) {
          return p.pfn().ToAddr() < max_addr;
        });
    if (it != list.end()) {
      fp = &*it;
      list.erase(it);
      break;
    }
    ++this_order;
  }
  if (fp == nullptr) {
    return Pfn::None();
  }

  while (this_order != order) {
    --this_order;
    FreePageNoCoalesce(fp->GetBuddy(this_order), this_order);
  }

  auto pfn = fp->pfn();
  auto page = mem_map::PfnToPage(fp->pfn());
  kassert(page != nullptr);
  page->usage = mem_map::Page::Usage::kInUse;
#ifdef PAGE_CHECKER
  kassert(AllocateAndCheck(pfn, order));
  kassert(Validate());
#endif
  return pfn;
}

ebbrt::Pfn ebbrt::PageAllocator::Alloc(size_t order, Nid nid,
                                       uint64_t max_addr) {
  if (nid == nid_) {
    return AllocLocal(order, max_addr);
  } else {
    return (*allocators)[nid.val()].AllocLocal(order, max_addr);
  }
}

void ebbrt::PageAllocator::FreePageNoCoalesce(Pfn pfn, size_t order) {
  auto entry = PfnToFreePage(pfn);
  free_page_lists[order].push_front(*entry);
  auto page = mem_map::PfnToPage(pfn);
  kassert(page != nullptr);
  page->usage = mem_map::Page::Usage::kPageAllocator;
  page->data.order = order;
}

void ebbrt::PageAllocator::Free(Pfn pfn, size_t order) {
  std::lock_guard<SpinLock> lock(lock_);
#ifdef PAGE_CHECKER
  kassert(Release(pfn, order));
#endif
  kassert(order <= kMaxOrder);
  while (order < kMaxOrder) {
    auto buddy = PfnToBuddy(pfn, order);
    auto page = mem_map::PfnToPage(buddy);
    if (page == nullptr ||
        page->usage != mem_map::Page::Usage::kPageAllocator ||
        page->data.order != order) {
      break;
    }
    // Buddy is free, remove it from the buddy system
    auto entry = reinterpret_cast<FreePage*>(buddy.ToAddr());
    auto it = free_page_lists[order].iterator_to(*entry);
    free_page_lists[order].erase(it);
    order++;
    pfn = std::min(pfn, buddy);
  }
  FreePageNoCoalesce(pfn, order);
#ifdef PAGE_CHECKER
  kassert(Validate());
#endif
}

#ifdef PAGE_CHECKER

bool ebbrt::PageAllocator::Validate() const {
  for (size_t i = 0; i <= kMaxOrder; ++i) {
    const auto& list = free_page_lists[i];
    for (const auto& free_page : list) {
      auto pfn = free_page.pfn();
      auto page = mem_map::PfnToPage(pfn);
      if (page->usage != mem_map::Page::Usage::kPageAllocator)
        return false;
      if (page->data.order != i) {
        return false;
      }
      // Check that no other page in this range is in the buddy allocator
      for (size_t j = 0; j <= kMaxOrder; ++j) {
        const auto& list = free_page_lists[j];
        for (const auto& free_page : list) {
          auto other_pfn = free_page.pfn();
          if (!(pfn == other_pfn && i == j) &&
              ((pfn <= other_pfn && other_pfn < (pfn + (1 << i))) ||
               (other_pfn <= pfn && pfn < (other_pfn + (1 << j))))) {
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool ebbrt::PageAllocator::AllocateAndCheck(Pfn pfn, size_t order) {
  Allocation allocation;
  allocation.pfn = pfn;
  allocation.npages = 1 << order;
  auto it =
      std::lower_bound(allocations_.begin(), allocations_.end(), allocation);
  if (it != allocations_.end() && !(allocation < *it)) {
    return false;
  }
  allocations_.insert(it, std::move(allocation));
  return true;
}

bool ebbrt::PageAllocator::Release(Pfn pfn, size_t order) {
  Allocation allocation;
  allocation.pfn = pfn;
  allocation.npages = 1 << order;
  auto it = std::find(allocations_.begin(), allocations_.end(), allocation);
  if (it == allocations_.end()) {
    return false;
  }
  allocations_.erase(it);
  return true;
}
#endif
