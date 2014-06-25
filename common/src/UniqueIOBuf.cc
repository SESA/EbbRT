//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/UniqueIOBuf.h>

ebbrt::UniqueIOBufOwner::UniqueIOBufOwner(size_t capacity, bool zero_memory) {
  void* b;
  if (zero_memory) {
    b = std::calloc(capacity, 1);
  } else {
    b = std::malloc(capacity);
  }

  if (b == nullptr)
    throw std::bad_alloc();

  ptr_.reset(static_cast<uint8_t*>(b));
  capacity_ = capacity;
}

const uint8_t* ebbrt::UniqueIOBufOwner::Buffer() const { return ptr_.get(); }

size_t ebbrt::UniqueIOBufOwner::Capacity() const { return capacity_; }
