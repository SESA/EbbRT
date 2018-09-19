//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_DEBUG_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_DEBUG_H_

#include <stdarg.h>

#include <cstdio>
#include <utility>

#include "Preprocessor.h"

namespace ebbrt {
__attribute__((no_instrument_function)) void
kvprintf(const char* __restrict format, va_list va);
__attribute__((no_instrument_function)) void
kprintf(const char* __restrict format, ...);
__attribute__((no_instrument_function)) void
kprintf_force(const char* __restrict format, ...);

#if __cplusplus > 199711L
template <typename... Args>
__attribute__((noreturn)) void kabort(const Args&... args) {
  kprintf_force(args...);
  kabort();
}

template <> __attribute__((noreturn)) inline void kabort() {
  kprintf_force("Aborting!\n");
  while (true) {
    asm volatile("cli;"
                 "hlt;"
                 :
                 :
                 : "memory");
  }
}

#define EBBRT_UNIMPLEMENTED()                                                  \
  do {                                                                         \
    ebbrt::kabort("%s: unimplemented\n", __PRETTY_FUNCTION__);                 \
  } while (0)

#ifndef NDEBUG
#define kassert(expr)                                                          \
  do {                                                                         \
    if (!(expr)) {                                                             \
      ebbrt::kabort("Assertion failed: " __FILE__                              \
                    ", line " STR2(__LINE__) ": " #expr "\n");                 \
    }                                                                          \
  } while (0)
#else
#define kassert(expr)                                                          \
  do {                                                                         \
    (void)sizeof(expr);                                                        \
  } while (0)
#endif

template <typename... Args> void kbugon(bool expr, const Args&... args) {
  if (expr) {
    kabort(args...);
  }
}
#endif
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_DEBUG_H_
