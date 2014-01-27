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

boost::container::static_vector<ebbrt::PageAllocator, ebbrt::numa::kMaxNodes>
    ebbrt::PageAllocator::allocators;

void ebbrt::PageAllocator::Init() {
  for (unsigned i = 0; i < numa::nodes.size(); ++i) {
    allocators.emplace_back(Nid(i));
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
}

void ebbrt::PageAllocator::EarlyFreePage(Pfn start, size_t order, Nid nid) {
  kassert(order <= kMaxOrder);
  auto entry = PfnToFreePage(start);
  allocators[nid.val()].free_page_lists[order].push_front(*entry);
  auto page = mem_map::PfnToPage(start);
  kassert(page != nullptr);
  page->usage = mem_map::Page::Usage::kPageAllocator;
  page->data.order = order;
}

ebbrt::PageAllocator& ebbrt::PageAllocator::HandleFault(EbbId id) {
  kassert(id == kPageAllocatorId);
  auto& allocator = allocators[Cpu::GetMyNode().val()];
  EbbRef<PageAllocator>::CacheRef(id, allocator);
  return allocator;
}

ebbrt::PageAllocator::PageAllocator(Nid nid) : nid_(nid) {}

ebbrt::Pfn ebbrt::PageAllocator::AllocLocal(size_t order) {
  std::lock_guard<SpinLock> lock(lock_);

  FreePage* fp = nullptr;
  auto this_order = order;
  while (this_order <= kMaxOrder) {
    auto& list = free_page_lists[this_order];
    if (!list.empty()) {
      fp = &list.front();
      list.pop_front();
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
  return pfn;
}

ebbrt::Pfn ebbrt::PageAllocator::Alloc(size_t order, Nid nid) {
  if (nid == nid_) {
    return AllocLocal(order);
  } else {
    return allocators[nid.val()].AllocLocal(order);
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
  }
  FreePageNoCoalesce(pfn, order);
}
