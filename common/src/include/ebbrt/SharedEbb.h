//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_SHAREDEBB_H_
#define COMMON_SRC_INCLUDE_EBBRT_SHAREDEBB_H_

#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/LocalIdMap.h>

namespace ebbrt {
template <class T> class SharedEbb {
 public:
  template <typename... CreateArgs>
  static ebbrt::Future<ebbrt::EbbRef<T>> Create(CreateArgs&&... create_args) {
    return CreateWithId(ebbrt::ebb_allocator->Allocate(),
                        std::forward<CreateArgs>(create_args)...);
  }
  template <typename... CreateArgs>
  static ebbrt::Future<ebbrt::EbbRef<T>>
  CreateWithId(ebbrt::EbbId id, CreateArgs&&... create_args) {
    return T::PreCreate(id, std::forward<CreateArgs>(create_args)...)
        .Then([id](ebbrt::Future<void> f) {
          f.Get();
          return ebbrt::EbbRef<T>(id);
        });
  }

  static T& HandleFault(EbbId id) {
    {
      ebbrt::LocalIdMap::ConstAccessor accessor;
      auto found = ebbrt::local_id_map->Find(accessor, id);
      if (found) {
        auto& m = *boost::any_cast<T*>(accessor->second);
        ebbrt::EbbRef<T>::CacheRef(id, m);
        return m;
      }
    }
    // try to construct
    auto& p = T::CreateRep(id);

    auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, &p));
    if (inserted) {
      ebbrt::EbbRef<T>::CacheRef(id, p);
      return p;
    }

    // raced, delete the new T
    T::DestroyRep(id, p);
    ebbrt::LocalIdMap::ConstAccessor accessor;
    auto found = ebbrt::local_id_map->Find(accessor, id);
    assert(found);
    (void)found;  // unused variable
    auto& m = *boost::any_cast<T*>(accessor->second);
    ebbrt::EbbRef<T>::CacheRef(id, m);
    return m;
  }
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_SHAREDEBB_H_
