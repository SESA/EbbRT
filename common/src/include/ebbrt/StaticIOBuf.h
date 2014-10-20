//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_STATICIOBUF_H_
#define COMMON_SRC_INCLUDE_EBBRT_STATICIOBUF_H_

#include <cstdlib>

#include <ebbrt/IOBuf.h>

namespace ebbrt {
class StaticIOBufOwner {
 public:
  explicit StaticIOBufOwner(const char* s) noexcept;
  StaticIOBufOwner(const uint8_t* ptr, size_t len) noexcept;
  template <typename T>
  explicit StaticIOBufOwner(const T& ref) noexcept
      : StaticIOBufOwner(reinterpret_cast<const uint8_t*>(&ref), sizeof(T)) {}

  const uint8_t* Buffer() const;
  size_t Capacity() const;

 private:
  const uint8_t* ptr_;
  size_t capacity_;
};

typedef IOBufBase<StaticIOBufOwner> StaticIOBuf;
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_STATICIOBUF_H_
