//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <cstdarg>
#include <cstdio>

#include "Debug.h"

void ebbrt::kprintf(const char* __restrict format, ...) {
#ifndef __EBBRT_QUIET__
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
#endif
}
