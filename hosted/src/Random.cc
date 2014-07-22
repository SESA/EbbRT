//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Random.h>

#include <random>

#include <ebbrt/ExplicitlyConstructed.h>

namespace {
ebbrt::ExplicitlyConstructed<std::mt19937> gen;
ebbrt::ExplicitlyConstructed<std::uniform_int_distribution<uint64_t>> dis;
}

#include <time.h>

void ebbrt::random::Init() {
  gen.construct(time(NULL));
  dis.construct();
}

uint64_t ebbrt::random::Get() { return (*dis)(*gen); }
