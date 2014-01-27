//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_CLOCK_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_CLOCK_H_

#include <chrono>

namespace ebbrt {
namespace clock {
void Init();
void ApInit();
std::chrono::nanoseconds Time();
}
}

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_CLOCK_H_
