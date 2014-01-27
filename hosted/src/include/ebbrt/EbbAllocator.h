//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_EBBALLOCATOR_H_
#define HOSTED_SRC_INCLUDE_EBBRT_EBBALLOCATOR_H_

#include <mutex>

#include <boost/icl/interval_set.hpp>

#include <ebbrt/CacheAligned.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {
class EbbAllocator : public StaticSharedEbb<EbbAllocator>, public CacheAligned {
  std::mutex mut_;
  boost::icl::interval_set<EbbId> free_ids_;

 public:
  EbbAllocator();
  EbbId AllocateLocal();
};

const constexpr auto ebb_allocator = EbbRef<EbbAllocator>(kEbbAllocatorId);
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_EBBALLOCATOR_H_
