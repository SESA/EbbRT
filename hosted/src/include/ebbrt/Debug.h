//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_DEBUG_H_
#define HOSTED_SRC_INCLUDE_EBBRT_DEBUG_H_

#include <cassert>
#include <stdlib.h>

#define kassert(expr) assert(expr)
#define EBBRT_UNIMPLEMENTED() kabort()

namespace ebbrt {

static __attribute__((noreturn)) void kabort() { abort(); }

template <typename... Args> void kprintf(Args&&... args) {
  printf(std::forward<Args>(args)...);  // NOLINT
}

template <typename... Args>
__attribute__((noreturn)) void kabort(const Args&... args) {
  kprintf(args...);
  kabort();
}

template <typename... Args> void kbugon(bool expr, const Args&... args) {
  if (expr) {
    kabort(args...);
  }
}

}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_DEBUG_H_
