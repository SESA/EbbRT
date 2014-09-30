//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_SHAREDIOBUFREF_H_
#define COMMON_SRC_INCLUDE_EBBRT_SHAREDIOBUFREF_H_

#include <cstdlib>

#include <ebbrt/IOBuf.h>

namespace ebbrt {
class SharedIOBufRefOwner;

typedef IOBufBase<SharedIOBufRefOwner> SharedIOBufRef;
typedef MutIOBufBase<SharedIOBufRefOwner> MutSharedIOBufRef;

class SharedIOBufRefOwner {
 public:
  explicit SharedIOBufRefOwner(std::unique_ptr<IOBuf>&& buf);

  const uint8_t* Buffer() const;
  size_t Capacity() const;

 private:
  std::shared_ptr<IOBuf> buf_;
};

}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_SHAREDIOBUFREF_H_
