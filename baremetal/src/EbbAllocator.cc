//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/EbbAllocator.h>

#include <mutex>

#include <ebbrt/Debug.h>
#include <ebbrt/ExplicitlyConstructed.h>

namespace {
ebbrt::ExplicitlyConstructed<ebbrt::EbbAllocator> the_allocator;
}

void ebbrt::EbbAllocator::Init() { the_allocator.construct(); }

ebbrt::EbbAllocator &ebbrt::EbbAllocator::HandleFault(EbbId id) {
  kassert(id == kEbbAllocatorId);
  auto &ref = *the_allocator;
  EbbRef<EbbAllocator>::CacheRef(id, ref);
  return ref;
}

ebbrt::EbbAllocator::EbbAllocator() {
  auto start_ids =
      boost::icl::interval<EbbId>::type(FIRST_FREE_ID, (1 << 16) - 1);
  free_ids_.insert(std::move(start_ids));
}

ebbrt::EbbId ebbrt::EbbAllocator::AllocateLocal() {
  std::lock_guard<SpinLock> lock(lock_);
  kbugon(free_ids_.empty());
  auto ret = boost::icl::first(free_ids_);
  free_ids_.erase(ret);
  return ret;
}
