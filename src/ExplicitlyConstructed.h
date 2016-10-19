//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_EXPLICITLYCONSTRUCTED_H_
#define COMMON_SRC_INCLUDE_EBBRT_EXPLICITLYCONSTRUCTED_H_

#include <new>
#include <type_traits>
#include <utility>

namespace ebbrt {
template <typename T> class ExplicitlyConstructed {
 public:
  template <typename... Args> void construct(Args&&... args) {
    ::new (&storage_) T(std::forward<Args>(args)...);
  }
  void destruct() {
    auto ptr = reinterpret_cast<T*>(&storage_);
    ptr->~T();
  }

  const T* operator->() const { return reinterpret_cast<T const*>(&storage_); }

  T* operator->() { return reinterpret_cast<T*>(&storage_); }

  const T& operator*() const { return *reinterpret_cast<T const*>(&storage_); }

  T& operator*() { return *reinterpret_cast<T*>(&storage_); }

  ExplicitlyConstructed() = default;
  ExplicitlyConstructed(const ExplicitlyConstructed&) = delete;
  ExplicitlyConstructed& operator=(const ExplicitlyConstructed&) = delete;
  ExplicitlyConstructed(ExplicitlyConstructed&&) = delete;
  ExplicitlyConstructed& operator=(const ExplicitlyConstructed&&) = delete;

 private:
  typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
} __attribute__((packed));
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_EXPLICITLYCONSTRUCTED_H_
