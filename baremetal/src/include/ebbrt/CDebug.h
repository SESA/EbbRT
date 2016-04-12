//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_CDEBUG_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_CDEBUG_H_

#ifdef __cplusplus
extern "C" __attribute__((noreturn)) void
ebbrt_kabort(const char* __restrict format, ...);
#else
__attribute__((noreturn)) void ebbrt_kabort(const char* __restrict format, ...);
#endif

#define EBBRT_UNIMPLEMENTED()                                                  \
  do {                                                                         \
    ebbrt_kabort("%s: unimplemented\n", __PRETTY_FUNCTION__);                  \
  } while (0)

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_CDEBUG_H_
