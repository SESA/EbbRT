//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/StaticIOBuf.h>

#include <cstring>

ebbrt::StaticIOBufOwner::StaticIOBufOwner(const char* s) noexcept {
  ptr_ = reinterpret_cast<const uint8_t*>(s);
  capacity_ = strlen(s);
}

ebbrt::StaticIOBufOwner::StaticIOBufOwner(const uint8_t* ptr,
                                          size_t len) noexcept {
  ptr_ = ptr;
  capacity_ = len;
}

const uint8_t* ebbrt::StaticIOBufOwner::Buffer() const { return ptr_; }

size_t ebbrt::StaticIOBufOwner::Capacity() const { return capacity_; }
