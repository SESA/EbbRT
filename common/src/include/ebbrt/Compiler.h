//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_COMPILER_H_
#define COMMON_SRC_INCLUDE_EBBRT_COMPILER_H_

#define likely(x) __builtin_expect(static_cast<bool>(x), 1)
#define unlikely(x) __builtin_expect(static_cast<bool>(x), 0)

#endif  // COMMON_SRC_INCLUDE_EBBRT_COMPILER_H_
