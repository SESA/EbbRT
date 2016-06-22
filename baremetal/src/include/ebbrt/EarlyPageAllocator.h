//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_EARLYPAGEALLOCATOR_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_EARLYPAGEALLOCATOR_H_

#include <boost/intrusive/set.hpp>
#include <boost/utility.hpp>

#include <ebbrt/ExplicitlyConstructed.h>
#include <ebbrt/Numa.h>
#include <ebbrt/Pfn.h>

namespace ebbrt {
namespace early_page_allocator {
void Init();
void ReserveRange(uint64_t begin, uint64_t length);
void FreeMemoryRange(uint64_t begin, uint64_t length, Nid nid = Nid::Any());
Pfn AllocatePage(unsigned int npages = 1, Nid nid = Nid::Any());
void FreePage(Pfn start, Pfn end, Nid nid = Nid::Any());
void SetNidRange(Pfn start, Pfn end, Nid nid);

class PageRange {
 public:
  explicit PageRange(size_t npages, Nid nid) noexcept : npages_(npages),
                                                        nid_(nid) {}

  Pfn start() const noexcept {
    return Pfn::Down(reinterpret_cast<uintptr_t>(this));
  }
  Pfn end() const noexcept { return start() + npages_; }
  size_t npages() const noexcept { return npages_; }
  Nid nid() const noexcept { return nid_; }

  void set_npages(size_t npages) noexcept { npages_ = npages; }
  void set_nid(Nid nid) noexcept { nid_ = nid; }

  boost::intrusive::set_member_hook<
      boost::intrusive::link_mode<boost::intrusive::normal_link>>
      member_hook;

 private:
  size_t npages_;
  Nid nid_;
};

inline bool operator<(const PageRange& lhs, const PageRange& rhs) noexcept {
  return lhs.start() < rhs.start();
}

typedef boost::intrusive::set<  // NOLINT
    PageRange,
    boost::intrusive::member_hook<
        PageRange,
        boost::intrusive::set_member_hook<
            boost::intrusive::link_mode<boost::intrusive::normal_link>>,
        &PageRange::member_hook>>
    FreePageTree;
extern ExplicitlyConstructed<FreePageTree> free_pages;

template <typename F> void ReleaseFreePages(F f) {
  auto it = free_pages->begin();
  auto end = free_pages->end();
  while (it != end) {
    auto start = it->start();
    auto end = it->end();
    auto nid = it->nid();
    // CAREFUL: advance the iterator before passing the free page, otherwise the
    // function may manipulate the page and trash our iterator
    auto next_it = boost::next(it);
    free_pages->erase(it);
    it = next_it;
    f(start, end, nid);
  }
}
}  // namespace early_page_allocator
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_EARLYPAGEALLOCATOR_H_
