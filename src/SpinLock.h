//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_SPINLOCK_H_
#define COMMON_SRC_INCLUDE_EBBRT_SPINLOCK_H_

#include <atomic>

namespace ebbrt {
class SpinLock {
 public:
  SpinLock() : lock_(ATOMIC_FLAG_INIT) {}
  // We use lowercase naming for these functions so they are compatible with STL
  // locks
  void lock() {
    while (lock_.test_and_set(std::memory_order_acquire)) {
    }
  }
  void unlock() { lock_.clear(std::memory_order_release); }

 private:
  std::atomic_flag lock_;
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_SPINLOCK_H_
