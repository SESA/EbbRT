//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/EbbAllocator.h>

ebbrt::EbbAllocator::EbbAllocator() {
  auto local_ids =
      boost::icl::interval<EbbId>::type(kFirstFreeId, kFirstStaticUserId - 1);
  free_ids_.insert(std::move(local_ids));
  auto global_start = 1 << 16;
  auto global_end = (2 << 16) - 1;
  auto global_ids = boost::icl::interval<EbbId>::type(global_start, global_end);
  free_global_ids_.insert(std::move(global_ids));
}

ebbrt::EbbId ebbrt::EbbAllocator::Allocate() {
  std::lock_guard<std::mutex> lock(mut_);
  if (free_global_ids_.empty())
    throw std::runtime_error("Out of Global Ids!");

  auto ret = boost::icl::first(free_global_ids_);
  free_global_ids_.erase(ret);
  return ret;
}
