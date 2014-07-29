//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_PAGEALLOCATOR_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_PAGEALLOCATOR_H_

// #define PAGE_CHECKER

#ifdef PAGE_CHECKER
#include <boost/container/static_vector.hpp>
#endif
#include <boost/intrusive/list.hpp>

#include <ebbrt/CacheAligned.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/Numa.h>
#include <ebbrt/Pfn.h>
#include <ebbrt/VMem.h>
#include <ebbrt/SpinLock.h>
#include <ebbrt/StaticIds.h>

namespace ebbrt {

class PageAllocator : public CacheAligned {
 public:
  static const constexpr size_t kMaxOrder = 11;

  explicit PageAllocator(Nid nid);

  static void Init();
  static PageAllocator& HandleFault(EbbId id);
  Pfn Alloc(size_t order = 0, Nid nid = Cpu::GetMyNode(),
            uint64_t max_addr = UINT64_MAX);
  void Free(Pfn pfn, size_t order = 0);

 private:
  class FreePage {
   public:
    Pfn pfn() const { return Pfn::Down(reinterpret_cast<uintptr_t>(this)); }
    Pfn GetBuddy(size_t order) const { return PfnToBuddy(pfn(), order); }

    boost::intrusive::list_member_hook<boost::intrusive::link_mode<
        boost::intrusive::normal_link>> member_hook_;
  };
  typedef boost::intrusive::list<  // NOLINT
      FreePage,
      boost::intrusive::member_hook<
          FreePage,
          boost::intrusive::list_member_hook<
              boost::intrusive::link_mode<boost::intrusive::normal_link>>,
          &FreePage::member_hook_>> FreePageList;

  static inline Pfn PfnToBuddy(Pfn pfn, size_t order) {
    return Pfn(pfn.val() ^ (1 << order));
  }

  static inline FreePage* PfnToFreePage(Pfn pfn) {
    auto addr = pfn.ToAddr();
    return new (reinterpret_cast<void*>(addr)) FreePage();
  }

  static void EarlyFreePage(Pfn start, size_t order, Nid nid);
  Pfn AllocLocal(size_t order, size_t max_addr);
  void FreePageNoCoalesce(Pfn pfn, size_t order);
#ifdef PAGE_CHECKER
  bool Validate() const;
  bool AllocateAndCheck(Pfn pfn, size_t order);
  bool Release(Pfn pfn, size_t order);
#endif

  static ExplicitlyConstructed<boost::container::static_vector<
      PageAllocator, numa::kMaxNodes>> allocators;
  SpinLock lock_;
  Nid nid_;
  std::array<FreePageList, kMaxOrder + 1> free_page_lists;

#ifdef PAGE_CHECKER
  struct Allocation {
    Pfn pfn;
    size_t npages;
    bool operator==(const Allocation& rhs) {
      return pfn == rhs.pfn && npages == rhs.npages;
    }
    bool operator<(const Allocation& rhs) { return (pfn + npages) <= rhs.pfn; }
  };
  static const constexpr size_t kMaxAllocations = 10000;
  boost::container::static_vector<Allocation, kMaxAllocations> allocations_;
#endif

  friend void ebbrt::vmem::ApInit(size_t index);
  friend void ebbrt::trans::ApInit(size_t index);
};

constexpr auto page_allocator = EbbRef<PageAllocator>(kPageAllocatorId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_PAGEALLOCATOR_H_
