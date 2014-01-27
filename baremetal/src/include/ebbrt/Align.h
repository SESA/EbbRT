//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_ALIGN_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_ALIGN_H_

#include <cstdint>
#include <cstring>

namespace ebbrt {
namespace align {
template <typename T, typename S> constexpr T Down(T val, S alignment) {
  return val & ~(alignment - 1);
}

template <typename T, typename S> constexpr T Up(T val, S alignment) {
  return Down(val + alignment - 1, alignment);
}

template <class T, typename S> T *Down(T *ptr, S alignment) {
  auto val = reinterpret_cast<uintptr_t>(ptr);
  return reinterpret_cast<T *>(Down(val, alignment));
}

template <class T, typename S> T *Up(T *ptr, S alignment) {
  auto val = reinterpret_cast<uintptr_t>(ptr);
  return reinterpret_cast<T *>(Up(val, alignment));
}
}  // namespace align
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_ALIGN_H_
