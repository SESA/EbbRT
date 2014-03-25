//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/EbbAllocator.h>

#include <mutex>

#include <ebbrt/Debug.h>

ebbrt::EbbAllocator::EbbAllocator() {
  auto start_ids =
      boost::icl::interval<EbbId>::type(kFirstFreeId, kFirstStaticUserId - 1);
  free_ids_.insert(std::move(start_ids));
}

ebbrt::EbbId ebbrt::EbbAllocator::AllocateLocal() {
  std::lock_guard<std::mutex> lock(lock_);
  kbugon(free_ids_.empty());
  auto ret = boost::icl::first(free_ids_);
  free_ids_.erase(ret);
  return ret;
}

void ebbrt::EbbAllocator::SetIdSpace(uint16_t space) {
  auto id_start = space << 16;
  auto id_end = ((space + 1) << 16) - 1;
  kprintf("Allocating Global EbbIds from %x - %x\n", id_start, id_end);
  auto start_ids = boost::icl::interval<EbbId>::type(id_start, id_end);
  free_global_ids_.insert(std::move(start_ids));
}
