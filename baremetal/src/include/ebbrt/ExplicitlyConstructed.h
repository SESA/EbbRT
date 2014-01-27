//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_EXPLICITLYCONSTRUCTED_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_EXPLICITLYCONSTRUCTED_H_

#include <type_traits>
#include <utility>

namespace ebbrt {
template <typename T> class ExplicitlyConstructed {
 public:
  template <typename... Args> void construct(Args&& ... args) {
    ::new (&storage_) T(std::forward<Args>(args)...);
  }
  void destruct() {
    auto ptr = static_cast<T*>(static_cast<void*>(&storage_));
    ptr->~T();
  }

  const T* operator->() const {
    return static_cast<T const*>(static_cast<void const*>(&storage_));
  }
  T* operator->() { return static_cast<T*>(static_cast<void*>(&storage_)); }

  const T& operator*() const {
    return *static_cast<T const*>(static_cast<void const*>(&storage_));
  }

  T& operator*() { return *static_cast<T*>(static_cast<void*>(&storage_)); }
  ExplicitlyConstructed() = default;
  ExplicitlyConstructed(const ExplicitlyConstructed&) = delete;
  ExplicitlyConstructed& operator=(const ExplicitlyConstructed&) = delete;
  ExplicitlyConstructed(ExplicitlyConstructed&&) = delete;
  ExplicitlyConstructed& operator=(const ExplicitlyConstructed&&) = delete;

 private:
  typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
} __attribute__((packed));
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_EXPLICITLYCONSTRUCTED_H_
