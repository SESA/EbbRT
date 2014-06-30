//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_ATOMICUNIQUEPTR_H_
#define COMMON_SRC_INCLUDE_EBBRT_ATOMICUNIQUEPTR_H_

#include <atomic>
#include <memory>

namespace ebbrt {
template <typename T, typename Deleter = std::default_delete<T>>
class atomic_unique_ptr {
  std::atomic<T*> ptr_;
  Deleter deleter_;

 public:
  explicit atomic_unique_ptr(T* ptr) : ptr_(ptr), deleter_(Deleter()) {}

  atomic_unique_ptr(atomic_unique_ptr&& other)
      : ptr_(other.exchange(nullptr).release()),
        deleter_(std::move(other.deleter_)) {}
  ~atomic_unique_ptr() {
    auto p = get();
    if (!p)
      deleter_(p);
  }
  T* get() { return ptr_.load(std::memory_order_consume); }
  void store(T* desired) {
    auto p = exchange(desired);
    if (!p)
      deleter_(p.release());
  }
  std::unique_ptr<T> exchange(T* desired) {
    return std::unique_ptr<T>(
        ptr_.exchange(desired, std::memory_order_release));
  }
};
}  // namespace ebbrt
#endif  // COMMON_SRC_INCLUDE_EBBRT_ATOMICUNIQUEPTR_H_
