//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_ARGV_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_ARGV_H_

#include <cstdint>
#include <cstdlib>

namespace ebbrt {
namespace argv {
const constexpr size_t kCmdLength = 256;

struct Data {
  char cmdline_string[kCmdLength];
  size_t len;
};
extern Data data;

size_t Init(const char* str);
}  // namespace argv
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_ARGV_H_
