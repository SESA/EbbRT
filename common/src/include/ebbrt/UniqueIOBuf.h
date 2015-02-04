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
  const uint8_t* Buffer() const;
  size_t Capacity() const;
  // delete must take into account the fact that the buffer and
  // descriptor(this) are allocated together
  void operator delete(void* ptr);

 private:
  // Private because it should not be called directly, use MakeUniqueIOBuf
  UniqueIOBufOwner(uint8_t* p, size_t capacity);

  uint8_t* ptr_;
  size_t capacity_;

  friend class IOBufBase<UniqueIOBufOwner>;
  friend class MutIOBufBase<UniqueIOBufOwner>;
};

// These template specializations ensure that the private constructor remains
// hidden
template <>
class IOBufBase<UniqueIOBufOwner> : public UniqueIOBufOwner, public IOBuf {
 public:
  const uint8_t* Buffer() const override { return UniqueIOBufOwner::Buffer(); }

  size_t Capacity() const override { return UniqueIOBufOwner::Capacity(); }

 private:
  IOBufBase(uint8_t* p, size_t capacity)
      : UniqueIOBufOwner(p, capacity), IOBuf(p, capacity) {}

  friend std::unique_ptr<MutUniqueIOBuf>
  ebbrt::MakeUniqueIOBuf(size_t capacity, bool zero_memory);
};

template <>
class MutIOBufBase<UniqueIOBufOwner> : public UniqueIOBufOwner,
                                       public MutIOBuf {
 public:
  const uint8_t* Buffer() const override { return UniqueIOBufOwner::Buffer(); }

  size_t Capacity() const override { return UniqueIOBufOwner::Capacity(); }

 private:
  MutIOBufBase(uint8_t* p, size_t capacity)
      : UniqueIOBufOwner(p, capacity), MutIOBuf(p, capacity) {}

  friend std::unique_ptr<MutUniqueIOBuf>
  ebbrt::MakeUniqueIOBuf(size_t capacity, bool zero_memory);
};

}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_UNIQUEIOBUF_H_
