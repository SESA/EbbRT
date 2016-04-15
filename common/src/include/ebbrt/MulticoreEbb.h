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
#include <ebbrt/EbbId.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/LocalIdMap.h>

namespace ebbrt {

template <class T> class MulticoreEbbRoot {
 public:
  struct RepMapHandle {
    RepMapHandle() : accessor_{std::make_unique<LocalIdMap::ConstAccessor>()} {}
    const boost::container::flat_map<size_t, T*>* operator->() const {
      return &operator*();
    }
    const boost::container::flat_map<size_t, T*>& operator*() const {
      auto pair = boost::any_cast<std::pair<MulticoreEbbRoot<T>*, RepMap>>(
          &(*accessor_)->second);
      return pair->second;
    }

   private:
    friend class MulticoreEbbRoot<T>;
    std::unique_ptr<LocalIdMap::ConstAccessor> accessor_;
  };
  MulticoreEbbRoot() = default;
  explicit MulticoreEbbRoot(EbbId id) : id_{id} {};

  void SetEbbId(EbbId id) { id_ = id; }
  RepMapHandle GetReps() const {
    if (!id_)
      throw std::runtime_error("No EbbRT specified in MulticoreEbbRoot");
    struct RepMapHandle rep;
    auto found = local_id_map->Find(*rep.accessor_, id_);
    if (!found)
      throw std::runtime_error("Failed to find root for MulticoreEbb");

    return std::move(rep);
  }

 private:
  LocalIdMap::ConstAccessor accessor_;
  typedef boost::container::flat_map<size_t, T*> RepMap;
  ebbrt::EbbId id_;
};

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

template <class T> class MulticoreEbb<T, MulticoreEbbRoot<T>> {
 public:
  void SetRoot(MulticoreEbbRoot<T>* root) { root_ = root; }
  const MulticoreEbbRoot<T>* GetRoot() { return root_; }
  static EbbRef<T> Create(MulticoreEbbRoot<T>* root,
                          EbbId id = ebb_allocator->Allocate()) {
    root->SetEbbId(id);
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

      auto pair = boost::any_cast<std::pair<MulticoreEbbRoot<T>*, RepMap>>(
          &accessor->second);
      const auto& rep_map = pair->second;
      auto root = pair->first;
      auto it = rep_map.find(Cpu::GetMine());
      if (it != rep_map.end()) {
        EbbRef<T>::CacheRef(id, *it->second);
        return *it->second;
      }
      // we failed to find a rep, we must construct one
      rep = new T();
      rep->SetRoot(root);
    }

    LocalIdMap::Accessor accessor;
    auto found = local_id_map->Find(accessor, id);
    if (!found)
      throw std::runtime_error("Failed to find root for MulticoreEbb");

    auto pair = boost::any_cast<std::pair<MulticoreEbbRoot<T>*, RepMap>>(
        &accessor->second);
    auto& rep_map = pair->second;
    rep_map[Cpu::GetMine()] = rep;
    EbbRef<T>::CacheRef(id, *rep);
    return *rep;
  }

 private:
  const MulticoreEbbRoot<T>* root_;
  typedef boost::container::flat_map<size_t, T*> RepMap;
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_MULTICOREEBB_H_
