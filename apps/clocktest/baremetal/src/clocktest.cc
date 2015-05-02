//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Clock.h>
#include <ebbrt/Debug.h>

void AppMain() {
  while (1) {
    auto t1 = ebbrt::clock::Wall::Now();
    while ((ebbrt::clock::Wall::Now() - t1) < std::chrono::seconds(1)) {
    }
    ebbrt::kprintf("Tick\n");
  }
}
