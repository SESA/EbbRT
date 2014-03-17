//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_CACHEALIGNED_H_
#define COMMON_SRC_INCLUDE_EBBRT_CACHEALIGNED_H_

#include <malloc.h>

#include <cstring>
#include <new>

namespace ebbrt {
const constexpr size_t cache_size = 64;

struct alignas(cache_size) CacheAligned {
#ifdef __EXCEPTIONS
  void* operator new(size_t size) {
    auto ret = memalign(cache_size, size);
    if (ret == nullptr) {
      throw std::bad_alloc();
    }
    return ret;
  }
  void operator delete(void* p) { free(p); }
#endif
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_CACHEALIGNED_H_
