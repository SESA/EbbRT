//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_FLS_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_FLS_H_

namespace ebbrt {
inline unsigned Fls(uint64_t word) {
  return 63 - __builtin_clzll(word);
}
}

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_FLS_H_
