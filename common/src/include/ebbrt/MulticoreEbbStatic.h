//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_MULTICOREEBBSTATIC_H_
#define COMMON_SRC_INCLUDE_EBBRT_MULTICOREEBBSTATIC_H_

#include <boost/container/flat_map.hpp>

#include <ebbrt/Cpu.h>
#include <ebbrt/Debug.h>
#include <ebbrt/LocalIdMap.h>

namespace ebbrt {
template <typename T> class MulticoreEbbStatic {
 public:
  static void Init() {
    local_id_map->Insert(std::make_pair(T::static_id, RepMap()));
  }
  static T& HandleFault(EbbId id) {
    kassert(id == T::static_id);
    {
      // acquire read only to find rep
      LocalIdMap::ConstAccessor accessor;
      auto found = local_id_map->Find(accessor, id);
      kassert(found);
      auto rep_map = boost::any_cast<RepMap>(accessor->second);
      auto it = rep_map.find(Cpu::GetMine());
      if (it != rep_map.end()) {
        EbbRef<T>::CacheRef(id, *it->second);
        return *it->second;
      }
    }
    // we failed to find a rep, we must construct one
    auto rep = new T;
    // TODO(dschatz): make the rep_map thread safe so we can acquire r/o access
    LocalIdMap::Accessor accessor;
    auto found = local_id_map->Find(accessor, id);
    kassert(found);
    auto rep_map = boost::any_cast<RepMap>(accessor->second);
    rep_map[Cpu::GetMine()] = rep;
    EbbRef<T>::CacheRef(id, *rep);
    return *rep;
  }

 private:
  typedef boost::container::flat_map<size_t, T*> RepMap;
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_MULTICOREEBBSTATIC_H_
