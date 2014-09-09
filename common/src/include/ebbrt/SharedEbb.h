//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_SHAREDEBB_H_
#define COMMON_SRC_INCLUDE_EBBRT_SHAREDEBB_H_

#include <utility>

#include <boost/container/flat_map.hpp>

#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/LocalIdMap.h>

namespace ebbrt {
template <class T> class SharedEbb {
 public:
  static EbbRef<T> Create(T* rep, EbbId id = ebb_allocator->Allocate()) {
    local_id_map->Insert(std::make_pair(id, std::move(rep)));
    return EbbRef<T>(id);
  }
  static T& HandleFault(EbbId id) {
    LocalIdMap::ConstAccessor accessor;
    auto found = local_id_map->Find(accessor, id);
    if (!found)
      throw std::runtime_error("Failed to find root for SharedEbb");

    auto rep = boost::any_cast<T*>(accessor->second);
    EbbRef<T>::CacheRef(id, *rep);
    return *rep;
  }
};

}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_SHAREDEBB_H_
