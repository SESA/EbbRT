//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_UNIQUEIOBUF_H_
#define COMMON_SRC_INCLUDE_EBBRT_UNIQUEIOBUF_H_

#include <cstdlib>

#include <ebbrt/IOBuf.h>

namespace ebbrt {
class UniqueIOBufOwner;

typedef IOBufBase<UniqueIOBufOwner> UniqueIOBuf;
typedef MutIOBufBase<UniqueIOBufOwner> MutUniqueIOBuf;

// This exists to allocate the IOBuf and data in one allocation
std::unique_ptr<MutUniqueIOBuf> MakeUniqueIOBuf(size_t capacity,
                                                bool zero_memory = false);

class UniqueIOBufOwner {
 public:
  UniqueIOBufOwner(uint8_t* p, size_t capacity);

  const uint8_t* Buffer() const;
  size_t Capacity() const;

 private:
  struct Deleter {
    void operator()(uint8_t* p);
  };

  uint8_t* ptr_;
  size_t capacity_;
};

}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_UNIQUEIOBUF_H_
