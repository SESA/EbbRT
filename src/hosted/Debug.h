//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_DEBUG_H_
#define HOSTED_SRC_INCLUDE_EBBRT_DEBUG_H_

#include <cassert>
#include <cstdlib>
#include <utility>

#ifndef NDEBUG
#define kassert(expr) assert(expr)
#else
#define kassert(expr)                                                          \
  do {                                                                         \
    (void)sizeof(expr);                                                        \
  } while (0)
#endif
#define EBBRT_UNIMPLEMENTED() kabort()

namespace ebbrt {

__attribute__((no_instrument_function)) void
kprintf(const char* __restrict format, ...);
__attribute__((no_instrument_function)) void
kprintf_force(const char* __restrict format, ...);

static __attribute__((noreturn)) void kabort() { abort(); }

template <typename... Args>
__attribute__((noreturn)) void kabort(Args&&... args) {
  kprintf_force(std::forward<Args>(args)...);  // NOLINT
  kabort();
}

template <typename... Args> void kbugon(bool expr, const Args&... args) {
  if (expr) {
    kabort(args...);
  }
}

}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_DEBUG_H_
