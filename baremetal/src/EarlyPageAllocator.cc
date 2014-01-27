//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/EarlyPageAllocator.h>

#include <cinttypes>
#include <new>

#include <boost/utility.hpp>

#include <ebbrt/Align.h>
#include <ebbrt/Debug.h>

ebbrt::ExplicitlyConstructed<ebbrt::early_page_allocator::FreePageTree>
    ebbrt::early_page_allocator::free_pages;
void ebbrt::early_page_allocator::Init() { free_pages.construct(); }

namespace {
typedef decltype(ebbrt::early_page_allocator::free_pages->begin())
    FreePageIterator;
FreePageIterator coalesce(FreePageIterator a, FreePageIterator b) {
  if (a->end() != b->start())
    return b;

  a->set_npages(a->npages() + b->npages());
  ebbrt::early_page_allocator::free_pages->erase(b);
  return a;
}

void FreePageRange(ebbrt::Pfn start, ebbrt::Pfn end, ebbrt::Nid nid) {
  auto range = new (reinterpret_cast<void*>(start.ToAddr()))
      ebbrt::early_page_allocator::PageRange(end - start, nid);

  auto it = ebbrt::early_page_allocator::free_pages->insert(*range).first;
  if (it != ebbrt::early_page_allocator::free_pages->begin()) {
    it = coalesce(boost::prior(it), it);
  }
  if (boost::next(it) != ebbrt::early_page_allocator::free_pages->end()) {
    coalesce(it, boost::next(it));
  }
}
}  // namespace

void ebbrt::early_page_allocator::FreeMemoryRange(uint64_t begin,
                                                  uint64_t length,
                                                  Nid nid) {
  auto pfn_start = Pfn::Up(begin);
  auto diff = pfn_start.ToAddr() - begin;
  if (diff > length)
    return;

  auto pfn_end = Pfn::Down(begin + length - diff);
  if (pfn_end == pfn_start)
    return;

  FreePageRange(pfn_start, pfn_end, nid);
}

ebbrt::Pfn ebbrt::early_page_allocator::AllocatePage(unsigned int npages,
                                                     Nid nid) {
  for (auto it = free_pages->begin(); it != free_pages->end(); ++it) {
    if (nid != Nid::Any() && it->nid() != nid)
      continue;

    auto ret = it->end() - npages;

    if (ret < it->start())
      continue;

    if (ret == it->start()) {
      free_pages->erase(it);
    } else {
      it->set_npages(it->npages() - npages);
    }
    // the iterator is potentially no longer valid, don't use it

    return ret;
  }

  kprintf("%s: unabled to allocate %" PRIu64 " pages\n",
          __PRETTY_FUNCTION__,
          npages);
  kabort();
}

void ebbrt::early_page_allocator::FreePage(Pfn start, Pfn end, Nid nid) {
  FreePageRange(start, end, nid);
}

void ebbrt::early_page_allocator::SetNidRange(Pfn start, Pfn end, Nid nid) {
  for (auto it = free_pages->begin(); it != free_pages->end(); ++it) {
    if (it->end() <= start)
      continue;

    if (it->start() >= end)
      return;

    if (it->start() < start && it->end() > start) {
      // straddles below
      auto new_end = std::min(end, it->end());
      auto new_range = new (reinterpret_cast<void*>(start.ToAddr()))
          PageRange(new_end - start, nid);
      free_pages->insert(*new_range);
      it->set_npages(start - it->start());
      continue;
    }

    if (it->start() < end && it->end() > end) {
      // straddles above
      auto new_range = new (reinterpret_cast<void*>(end.ToAddr()))
          PageRange(it->end() - end, Nid::Any());
      free_pages->insert(*new_range);
      it->set_npages(end - it->start());
    }

    kprintf("Early Page Allocator NUMA mapping %#018" PRIx64 "-%#018" PRIx64
            " -> %u\n",
            it->start().ToAddr(),
            it->end().ToAddr() - 1,
            nid.val());
    it->set_nid(nid);
  }
}
