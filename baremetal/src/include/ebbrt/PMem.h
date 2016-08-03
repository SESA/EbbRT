//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_PMEM_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_PMEM_H_

namespace ebbrt {
namespace pmem {
const constexpr size_t kPageShift = 12;
const constexpr size_t kPageSize = 1 << kPageShift;

const constexpr size_t kLargePageShift = 21;
const constexpr size_t kLargePageSize = 1 << kLargePageShift;
}
}

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_PMEM_H_
