//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_MAIN_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_MAIN_H_

#include <ebbrt/Multiboot.h>

namespace ebbrt {

extern "C" __attribute__((noreturn)) void
Main(ebbrt::multiboot::Information* mbi);
}

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_MAIN_H_
