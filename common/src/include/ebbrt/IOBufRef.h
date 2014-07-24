//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_IOBUFREF_H_
#define COMMON_SRC_INCLUDE_EBBRT_IOBUFREF_H_

#include <cstdlib>

#include <ebbrt/IOBuf.h>

namespace ebbrt {
class IOBufRefOwner {
 public:
  explicit IOBufRefOwner(const IOBuf& buf);

  const uint8_t* Buffer() const;
  size_t Capacity() const;

 private:
  const uint8_t* buffer_;
  size_t capacity_;
};

typedef IOBufBase<IOBufRefOwner> IOBufRef;
typedef MutIOBufBase<IOBufRefOwner> MutIOBufRef;

std::unique_ptr<IOBufRef> CreateRef(const IOBuf& buf);
std::unique_ptr<MutIOBufRef> CreateRef(const MutIOBuf& buf);
std::unique_ptr<IOBufRef> CreateRefChain(const IOBuf& buf);
std::unique_ptr<MutIOBufRef> CreateRefChain(const MutIOBuf& buf);
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_IOBUFREF_H_
