//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_EBBALLOCATOR_H_
#define COMMON_SRC_INCLUDE_EBBRT_EBBALLOCATOR_H_

#include <mutex>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#include <boost/icl/interval_set.hpp>
#pragma GCC diagnostic pop

#include "CacheAligned.h"
#include "EbbRef.h"
#include "Runtime.h"
#include "StaticIds.h"
#include "StaticSharedEbb.h"
#include "SpinLock.h"

namespace ebbrt {
class EbbAllocator : public CacheAligned, public StaticSharedEbb<EbbAllocator> {
 public:
  static void ClassInit() {} // no class wide static initialization logic
  
  EbbAllocator();
  EbbId AllocateLocal();
  EbbId Allocate();
#ifdef __ebbrt__
  void SetIdSpace(uint16_t id_space);
#endif

 private:
  ebbrt::SpinLock lock_;
  boost::icl::interval_set<EbbId> free_ids_;
  boost::icl::interval_set<EbbId> free_global_ids_;
};

constexpr auto ebb_allocator = EbbRef<EbbAllocator>(kEbbAllocatorId);
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_EBBALLOCATOR_H_
