//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_POOLALLOCATOR_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_POOLALLOCATOR_H_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#include <boost/icl/interval.hpp>
#include <boost/icl/interval_set.hpp>
#pragma GCC diagnostic pop
#include <boost/optional.hpp>

#include <ebbrt/CacheAligned.h>
#include <ebbrt/MulticoreEbb.h>

namespace ebbrt {
template <typename T, size_t target_pool_size>
class PoolAllocatorRoot : public CacheAligned {
 public:
  PoolAllocatorRoot(T begin, T end) {
    set_ += typename boost::icl::interval<T>::type(begin, end);
  }

  boost::icl::interval_set<T> Allocate() const {
    boost::icl::interval_set<T> ret;

    std::lock_guard<std::mutex> guard(lock_);
    RemoveFrom(set_, ret);
    return ret;
  }

  void Free(boost::icl::interval_set<T>& local) const {
    std::lock_guard<std::mutex> guard(lock_);
    RemoveFrom(local, set_);
  }

 private:
  void RemoveFrom(boost::icl::interval_set<T>& from,
                  boost::icl::interval_set<T>& to) const {
    auto to_add = target_pool_size;
    for (auto it = from.begin(); it != from.end(); ++it) {
      auto len = boost::icl::length(*it);
      if (len <= to_add) {
        // just take the entire interval
        to += *it;
        to_add -= len;
      } else {
        auto front = boost::icl::lower(*it);
        to += typename boost::icl::interval<T>::type(front, front + to_add);
        to_add = 0;
      }

      if (to_add == 0)
        break;
    }

    from -= to;
  }

  mutable std::mutex lock_;
  mutable boost::icl::interval_set<T> set_;
};

template <typename T, size_t target_pool_size>
class PoolAllocator
    : public MulticoreEbb<PoolAllocator<T, target_pool_size>,
                          PoolAllocatorRoot<T, target_pool_size>> {
 public:
  static EbbRef<PoolAllocator<T, target_pool_size>>
  Create(T begin, T end, EbbId id = ebb_allocator->Allocate()) {
    auto root = new PoolAllocatorRoot<T, target_pool_size>(begin, end);
    return MulticoreEbb<PoolAllocator<T, target_pool_size>,
                        PoolAllocatorRoot<T, target_pool_size>>::Create(root,
                                                                        id);
  }

  PoolAllocator(const PoolAllocatorRoot<T, target_pool_size>& root)
      : root_(root) {}

  boost::optional<T> Allocate() {
    if (unlikely(set_.empty())) {
      set_ = root_.Allocate();
      if (unlikely(set_.empty()))
        return boost::optional<T>();
    }

    auto ret = boost::icl::lower(set_);
    set_ -= ret;
    return boost::optional<T>(ret);
  }

  void Free(T val) {
    set_ += val;

    if (unlikely(boost::icl::length(set_) >= 2 * target_pool_size)) {
      root_.Free(set_);
    }
  }

 private:
  const PoolAllocatorRoot<T, target_pool_size>& root_;
  boost::icl::interval_set<T> set_;
};
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_POOLALLOCATOR_H_
