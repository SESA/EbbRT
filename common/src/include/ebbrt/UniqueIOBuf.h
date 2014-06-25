//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_UNIQUEIOBUF_H_
#define COMMON_SRC_INCLUDE_EBBRT_UNIQUEIOBUF_H_

#include <cstdlib>

#include <ebbrt/IOBuf.h>

namespace ebbrt {
class UniqueIOBufOwner {
 public:
  UniqueIOBufOwner(size_t capacity, bool zero_memory = false);

  const uint8_t* Buffer() const;
  size_t Capacity() const;

 private:
  struct Deleter {
    void operator()(uint8_t* p) { free(p); }
  };

  std::unique_ptr<uint8_t, Deleter> ptr_;
  size_t capacity_;
};

typedef IOBufBase<UniqueIOBufOwner> UniqueIOBuf;
typedef MutIOBufBase<UniqueIOBufOwner> MutUniqueIOBuf;
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_UNIQUEIOBUF_H_
