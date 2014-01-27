//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_STATICIDS_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_STATICIDS_H_

#include <ebbrt/EbbId.h>

namespace ebbrt {
enum : EbbId {
  kPageAllocatorId,
  kGpAllocatorId,
  kLocalIdMapId,
  kEbbAllocatorId,
  kEventManagerId,
  kVMemAllocatorId,
  kTimerId,
  kNetworkManagerId,
  FIRST_FREE_ID
};
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_STATICIDS_H_
