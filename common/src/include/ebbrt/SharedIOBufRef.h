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
  struct CloneView_s {};
  static const constexpr CloneView_s CloneView = {};
  explicit SharedIOBufRefOwner(std::unique_ptr<IOBuf>&& buf);

  const uint8_t* Buffer() const;
  size_t Capacity() const;

 protected:
  const IOBuf& GetRef() { return *buf_; }

 private:
  std::shared_ptr<IOBuf> buf_;
};

template <>
class IOBufBase<SharedIOBufRefOwner> : public SharedIOBufRefOwner,
                                       public IOBuf {
 public:
  template <typename... Args>
  explicit IOBufBase(Args&&... args)
      : SharedIOBufRefOwner(std::forward<Args>(args)...),
        IOBuf(SharedIOBufRefOwner::Buffer(), SharedIOBufRefOwner::Capacity()) {}

  template <typename... Args>
  explicit IOBufBase(SharedIOBufRefOwner::CloneView_s, Args&&... args)
      : SharedIOBufRefOwner(std::forward<Args>(args)...),
        IOBuf(SharedIOBufRefOwner::Buffer(), SharedIOBufRefOwner::Capacity()) {
    IOBuf::SetView(SharedIOBufRefOwner::GetRef());
  }

  IOBufBase(SharedIOBufRefOwner::CloneView_s, const SharedIOBufRef& r)
      : SharedIOBufRefOwner(r),
        IOBuf(SharedIOBufRefOwner::Buffer(), SharedIOBufRefOwner::Capacity()) {
    IOBuf::SetView(r);
  }

  IOBufBase(SharedIOBufRefOwner::CloneView_s, SharedIOBufRef& r)
      : SharedIOBufRefOwner(r),
        IOBuf(SharedIOBufRefOwner::Buffer(), SharedIOBufRefOwner::Capacity()) {
    IOBuf::SetView(r);
  }

  const uint8_t* Buffer() const override {
    return SharedIOBufRefOwner::Buffer();
  }

  size_t Capacity() const override { return SharedIOBufRefOwner::Capacity(); }
};

template <>
class MutIOBufBase<SharedIOBufRefOwner> : public SharedIOBufRefOwner,
                                          public MutIOBuf {
 public:
  template <typename... Args>
  explicit MutIOBufBase(Args&&... args)
      : SharedIOBufRefOwner(std::forward<Args>(args)...),
        MutIOBuf(SharedIOBufRefOwner::Buffer(),
                 SharedIOBufRefOwner::Capacity()) {}

  template <typename... Args>
  explicit MutIOBufBase(SharedIOBufRefOwner::CloneView_s, Args&&... args)
      : SharedIOBufRefOwner(std::forward<Args>(args)...),
        MutIOBuf(SharedIOBufRefOwner::Buffer(),
                 SharedIOBufRefOwner::Capacity()) {
    MutIOBuf::SetView(SharedIOBufRefOwner::GetRef());
  }
  MutIOBufBase(SharedIOBufRefOwner::CloneView_s, const MutSharedIOBufRef& r)
      : SharedIOBufRefOwner(r), MutIOBuf(SharedIOBufRefOwner::Buffer(),
                                         SharedIOBufRefOwner::Capacity()) {
    MutIOBuf::SetView(r);
  }

  MutIOBufBase(SharedIOBufRefOwner::CloneView_s, MutSharedIOBufRef& r)
      : SharedIOBufRefOwner(r), MutIOBuf(SharedIOBufRefOwner::Buffer(),
                                         SharedIOBufRefOwner::Capacity()) {
    MutIOBuf::SetView(r);
  }
  const uint8_t* Buffer() const override {
    return SharedIOBufRefOwner::Buffer();
  }

  size_t Capacity() const override { return SharedIOBufRefOwner::Capacity(); }
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_SHAREDIOBUFREF_H_
