//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_SPINBARRIER_H_
#define COMMON_SRC_INCLUDE_EBBRT_SPINBARRIER_H_

#include <atomic>

namespace ebbrt {
class SpinBarrier {
 public:
  explicit SpinBarrier(unsigned int n) : n_(n), waiters_(0), completed_(0) {}

  void Wait() {
    auto step = completed_.load();

    if (waiters_.fetch_add(1) == n_ - 1) {
      waiters_.store(0);
      completed_.fetch_add(1);
    } else {
      while (completed_.load() == step) {
      }
    }
  }

 private:
  const unsigned int n_;
  std::atomic<unsigned int> waiters_;
  std::atomic<unsigned int> completed_;
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_SPINBARRIER_H_
