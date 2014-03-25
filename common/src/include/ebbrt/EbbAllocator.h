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

#include <ebbrt/CacheAligned.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {
class EbbAllocator : public CacheAligned,
		     public StaticSharedEbb<EbbAllocator> {
 public:
  EbbAllocator();
  EbbId AllocateLocal();

 private:
  void SetIdSpace(uint16_t id_space);

  std::mutex lock_;
  boost::icl::interval_set<EbbId> free_ids_;
  boost::icl::interval_set<EbbId> free_global_ids_;

  friend void ebbrt::runtime::Init();
};

constexpr auto ebb_allocator = EbbRef<EbbAllocator>(kEbbAllocatorId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_EBBALLOCATOR_H_
