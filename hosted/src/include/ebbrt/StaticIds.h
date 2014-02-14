//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_STATICIDS_H_
#define HOSTED_SRC_INCLUDE_EBBRT_STATICIDS_H_

#include <ebbrt/EbbId.h>
#include <ebbrt/GlobalStaticIds.h>

namespace ebbrt {
enum : EbbId {
  kLocalIdMapId = kFirstLocalId,
  kEbbAllocatorId,
  kNetworkManagerId,
  kNodeAllocatorId,
  kMessengerId,
  kEventManagerId,
  kGlobalMapId,
  kFirstFreeId
};
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_STATICIDS_H_
