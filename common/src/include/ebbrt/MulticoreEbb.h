//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_MULTICOREEBB_H_
#define COMMON_SRC_INCLUDE_EBBRT_MULTICOREEBB_H_

#include <utility>

#include <boost/container/flat_map.hpp>

#include <ebbrt/Cpu.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/LocalIdMap.h>

namespace ebbrt {
template <class T, class S> class MulticoreEbb {
 public:
  static EbbRef<T> Create(S s) {
    auto id = ebb_allocator->AllocateLocal();
    auto root = std::make_pair(RepMap(), std::move(s));
    local_id_map->Insert(std::make_pair(id, std::move(root)));
    return EbbRef<T>(id);
  }
  static T& HandleFault(EbbId id) {
    {
      LocalIdMap::ConstAccessor accessor;
      auto found = local_id_map->Find(accessor, id);
      if (!found)
        throw std::runtime_error("Failed to find root for MulticoreEbb");

      auto root = boost::any_cast<Root>(accessor->second);
      auto& rep_map = root.first;
      auto it = rep_map.find(Cpu::GetMine());
      if (it != rep_map.end()) {
        EbbRef<T>::CacheRef(id, it->second.get());
        return it->second.get();
      }
    }
    // we failed to find a rep, we must construct one
    auto rep = new T;
    LocalIdMap::Accessor accessor;
    auto found = local_id_map->Find(accessor, id);
    if (!found)
      throw std::runtime_error("Failed to find root for MulticoreEbb");

    auto root = boost::any_cast<Root>(accessor->second);
    auto& rep_map = root.first;
    rep_map[Cpu::GetMine()] = *rep;
    EbbRef<T>::CacheRef(id, *rep);
    return *rep;
  }

 private:
  typedef boost::container::flat_map<size_t, std::reference_wrapper<T>> RepMap;
  typedef std::pair<RepMap, S> Root;
};

}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_MULTICOREEBB_H_
