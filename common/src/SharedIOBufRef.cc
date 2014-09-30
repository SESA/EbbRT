//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/SharedIOBufRef.h>

ebbrt::SharedIOBufRefOwner::SharedIOBufRefOwner(std::unique_ptr<IOBuf>&& buf)
    : buf_(std::move(buf)) {}

const uint8_t* ebbrt::SharedIOBufRefOwner::Buffer() const {
  return buf_->Buffer();
}

size_t ebbrt::SharedIOBufRefOwner::Capacity() const { return buf_->Capacity(); }
