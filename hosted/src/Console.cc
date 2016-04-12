//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ebbrt/Console.h>

void ebbrt::console::Init() noexcept {}

void ebbrt::console::Write(const char* str) noexcept {
  size_t len = strlen(str);
  int n = 0;
  do {
    n = write(STDOUT_FILENO, str, len);
    assert(n > 0);
    len -= n;
  } while (len > 0);
}

int ebbrt::console::Write(const char* buf, int len) noexcept {
  return write(STDOUT_FILENO, buf, len);
}
