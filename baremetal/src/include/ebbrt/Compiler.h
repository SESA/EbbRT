//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_COMPILER_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_COMPILER_H_

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_COMPILER_H_
