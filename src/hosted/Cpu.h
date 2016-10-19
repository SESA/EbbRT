//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_CPU_H_
#define HOSTED_SRC_INCLUDE_EBBRT_CPU_H_

#include <stddef.h>
#include <stdint.h>

#include "Context.h"

namespace ebbrt {

class Cpu {
 public:
  static size_t GetMine() { return active_context->GetIndex(); }
};
}

#endif  // HOSTED_SRC_INCLUDE_EBBRT_CPU_H_
