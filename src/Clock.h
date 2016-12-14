//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_CLOCK_H_
#define COMMON_SRC_INCLUDE_EBBRT_CLOCK_H_

#ifdef __ebbrt__
#include "native/Clock.h"
#else
#include "hosted/Clock.h"
#endif

#endif  // COMMON_SRC_INCLUDE_EBBRT_CLOCK_H_
