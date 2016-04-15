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
template <class T, class Root = void> class MulticoreEbb;
template <class T, class Root> class MulticoreEbb {
 public:
  static EbbRef<T> Create(Root* root, EbbId id = ebb_allocator->Allocate()) {
    auto pair = std::make_pair(root, RepMap());
    local_id_map->Insert(std::make_pair(id, std::move(pair)));
    return EbbRef<T>(id);
  }
  static T& HandleFault(EbbId id) {
    T* rep;
    {
      LocalIdMap::ConstAccessor accessor;
      auto found = local_id_map->Find(accessor, id);
      if (!found)
        throw std::runtime_error("Failed to find root for MulticoreEbb");

      auto pair = boost::any_cast<std::pair<Root*, RepMap>>(&accessor->second);
      const auto& root = *(pair->first);
      const auto& rep_map = pair->second;
      auto it = rep_map.find(Cpu::GetMine());
      if (it != rep_map.end()) {
        EbbRef<T>::CacheRef(id, *it->second);
        return *it->second;
      }
      // we failed to find a rep, we must construct one
      rep = new T(root);
    }

    LocalIdMap::Accessor accessor;
    auto found = local_id_map->Find(accessor, id);
    if (!found)
      throw std::runtime_error("Failed to find root for MulticoreEbb");

    auto pair = boost::any_cast<std::pair<Root*, RepMap>>(&accessor->second);
    auto& rep_map = pair->second;
    rep_map[Cpu::GetMine()] = rep;
    EbbRef<T>::CacheRef(id, *rep);
    return *rep;
  }

 private:
  typedef boost::container::flat_map<size_t, T*> RepMap;
};

template <class T> class MulticoreEbb<T, void> {
 public:
  static EbbRef<T> Create(EbbId id = ebb_allocator->Allocate()) {
    local_id_map->Insert(std::make_pair(id, RepMap()));
    return EbbRef<T>(id);
  }

  static T& HandleFault(EbbId id) {
    T* rep;
    {
      LocalIdMap::ConstAccessor accessor;
      auto found = local_id_map->Find(accessor, id);
      if (!found)
        throw std::runtime_error("Failed to find root for MulticoreEbb");
      auto rep_map = boost::any_cast<RepMap>(accessor->second);
      auto it = rep_map.find(Cpu::GetMine());
      if (it != rep_map.end()) {
        EbbRef<T>::CacheRef(id, *it->second);
        return *it->second;
      }
    }
    // we failed to find a rep, we must construct one
    rep = new T;
    LocalIdMap::Accessor accessor;
    auto found = local_id_map->Find(accessor, id);
    if (!found)
      throw std::runtime_error("Failed to find root for MulticoreEbb");
    auto rep_map = boost::any_cast<RepMap>(accessor->second);
    rep_map[Cpu::GetMine()] = rep;
    EbbRef<T>::CacheRef(id, *rep);
    return *rep;
  }

 private:
  typedef boost::container::flat_map<size_t, T*> RepMap;
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_MULTICOREEBB_H_
