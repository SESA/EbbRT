//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/EarlyPageAllocator.h>

#include <cinttypes>
#include <new>

#include <boost/container/static_vector.hpp>
#include <boost/utility.hpp>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#include <boost/icl/interval.hpp>
#pragma GCC diagnostic pop

#include <ebbrt/Align.h>
#include <ebbrt/Debug.h>

ebbrt::ExplicitlyConstructed<ebbrt::early_page_allocator::FreePageTree>
    ebbrt::early_page_allocator::free_pages;

namespace {
ebbrt::ExplicitlyConstructed<boost::container::static_vector<
    boost::icl::right_open_interval<uint64_t>, 256>>
    reserved_ranges;
}
void ebbrt::early_page_allocator::Init() {
  free_pages.construct();
  reserved_ranges.construct();
}

namespace {
typedef decltype(
    ebbrt::early_page_allocator::free_pages->begin()) FreePageIterator;

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
                                                  uint64_t length, Nid nid) {
  auto interval =
      boost::icl::right_open_interval<uint64_t>(begin, begin + length);
  for (auto it = reserved_ranges->begin(); it != reserved_ranges->end(); ++it) {
    if (!boost::icl::intersects(interval, *it))
      continue;
    auto left_over = left_subtract(interval, *it);
    auto right_over = right_subtract(interval, *it);
    auto left_empty = boost::icl::is_empty(left_over);
    auto right_empty = boost::icl::is_empty(right_over);
    if (left_empty && right_empty)
      return;

    if (left_empty) {
      interval = std::move(right_over);
    } else if (right_empty) {
      interval = std::move(left_over);
    } else {
      interval = std::move(left_over);
      FreeMemoryRange(boost::icl::lower(right_over),
                      boost::icl::size(right_over), nid);
    }
  }

  if (boost::icl::size(interval) < pmem::kPageSize)
    return;

  auto pfn_start = Pfn::Up(boost::icl::lower(interval));
  auto pfn_end = Pfn::Down(boost::icl::upper(interval));
  if (pfn_end <= pfn_start)
    return;

  kprintf("Early Page Allocator Free %#018" PRIx64 "-%#018" PRIx64 "\n",
          pfn_start.ToAddr(), pfn_end.ToAddr() - 1);

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

  kprintf("%s: unabled to allocate %" PRIu64 " pages\n", __PRETTY_FUNCTION__,
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
            it->start().ToAddr(), it->end().ToAddr() - 1, nid.val());
    it->set_nid(nid);
  }
}

void ebbrt::early_page_allocator::ReserveRange(uint64_t begin, uint64_t end) {
  kprintf("Early Page Allocator Reserve %#018" PRIx64 "-%#018" PRIx64 "\n",
          begin, end - 1);
  reserved_ranges->emplace_back(begin, end);
}
