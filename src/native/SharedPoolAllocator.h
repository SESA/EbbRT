//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_SHAREDPOOLALLOCATOR_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_SHAREDPOOLALLOCATOR_H_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#include <boost/icl/interval.hpp>
#include <boost/icl/interval_set.hpp>
#pragma GCC diagnostic pop
#include <boost/optional.hpp>

#include "../CacheAligned.h"
#include "../SharedEbb.h"

namespace ebbrt {
template <typename T>
class SharedPoolAllocator : public SharedEbb<SharedPoolAllocator<T>>,
                            public CacheAligned {
 public:
  static EbbRef<SharedPoolAllocator<T>>
  Create(T begin, T end, EbbId id = ebb_allocator->Allocate()) {
    return SharedEbb<SharedPoolAllocator<T>>::Create(
        new SharedPoolAllocator(begin, end), id);
  }

  SharedPoolAllocator(T begin, T end) {
    set_ += typename boost::icl::interval<T>::type(begin, end);
  }

  boost::optional<T> Allocate() {
    std::lock_guard<ebbrt::SpinLock> guard(lock_);
    if (unlikely(set_.empty())) {
      return boost::optional<T>();
    }

    auto ret = boost::icl::lower(set_);
    set_ -= ret;
    return boost::optional<T>(ret);
  }

  bool Reserve(const T& val) {
    std::lock_guard<ebbrt::SpinLock> guard(lock_);
    if (boost::icl::contains(set_, val)) {
      set_ -= val;
      return true;
    }
    return false;
  }

  void Free(T val) {
    std::lock_guard<ebbrt::SpinLock> guard(lock_);
    set_ += val;
  }

 private:
  ebbrt::SpinLock lock_;
  boost::icl::interval_set<T> set_;
};
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_SHAREDPOOLALLOCATOR_H_
