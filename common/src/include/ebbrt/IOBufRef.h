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
  struct CloneView_s {};
  static const constexpr CloneView_s CloneView = {};
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

template <>
class IOBufBase<IOBufRefOwner> : public IOBufRefOwner, public IOBuf {
 public:
  explicit IOBufBase(const IOBuf& buf)
      : IOBufRefOwner(buf),
        IOBuf(IOBufRefOwner::Buffer(), IOBufRefOwner::Capacity()) {}

  IOBufBase(IOBufRefOwner::CloneView_s, const IOBuf& buf)
      : IOBufRefOwner(buf),
        IOBuf(IOBufRefOwner::Buffer(), IOBufRefOwner::Capacity()) {
    IOBuf::SetView(buf);
  }

  const uint8_t* Buffer() const override { return IOBufRefOwner::Buffer(); }

  size_t Capacity() const override { return IOBufRefOwner::Capacity(); }
};

template <>
class MutIOBufBase<IOBufRefOwner> : public IOBufRefOwner, public MutIOBuf {
 public:
  explicit MutIOBufBase(const IOBuf& buf)
      : IOBufRefOwner(buf),
        MutIOBuf(IOBufRefOwner::Buffer(), IOBufRefOwner::Capacity()) {}

  MutIOBufBase(IOBufRefOwner::CloneView_s, const IOBuf& buf)
      : IOBufRefOwner(buf),
        MutIOBuf(IOBufRefOwner::Buffer(), IOBufRefOwner::Capacity()) {
    MutIOBuf::SetView(buf);
  }

  const uint8_t* Buffer() const override { return IOBufRefOwner::Buffer(); }

  size_t Capacity() const override { return IOBufRefOwner::Capacity(); }
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_IOBUFREF_H_
