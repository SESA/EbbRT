//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Debug.h"

#include "../Console.h"

void ebbrt::kvprintf(const char* __restrict format, va_list va) {
  va_list va2;
  va_copy(va2, va);
  auto len = vsnprintf(nullptr, 0, format, va);
  char buffer[len + 1];  // NOLINT
  vsnprintf(buffer, len + 1, format, va2);
  console::Write(buffer);
}

void ebbrt::kprintf(const char* __restrict format, ...) {
#ifndef NDEBUG
  va_list ap;
  va_start(ap, format);
  kvprintf(format, ap);
  va_end(ap);
#endif
}

void ebbrt::kprintf_force(const char* __restrict format, ...) {
  va_list ap;
  va_start(ap, format);
  kvprintf(format, ap);
  va_end(ap);
}

extern "C" void ebbrt_kabort(const char* __restrict format, ...) {
  va_list ap;
  va_start(ap, format);
  ebbrt::kvprintf(format, ap);
  va_end(ap);
  ebbrt::kabort();
}
