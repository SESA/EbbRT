//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_CAPNPMESSAGE_H_
#define COMMON_SRC_INCLUDE_EBBRT_CAPNPMESSAGE_H_

#include <cstdlib>
#include <vector>

#include <capnp/message.h>

#include "Align.h"
#include "UniqueIOBuf.h"

namespace ebbrt {
class IOBufMessageBuilder : public capnp::MessageBuilder {
 public:
  kj::ArrayPtr<capnp::word> allocateSegment(capnp::uint minimumSize) override {
    auto buf = MakeUniqueIOBuf(minimumSize * sizeof(capnp::word), true);
    auto ptr = buf->MutData();

    if (buf_ == nullptr) {
      buf_ = std::move(buf);
    } else {
      buf_->PrependChain(std::move(buf));
    }

    return kj::arrayPtr(reinterpret_cast<capnp::word*>(ptr), minimumSize);
  }

  std::unique_ptr<MutIOBuf> Move() { return std::move(buf_); }

 private:
  std::unique_ptr<MutIOBuf> buf_;
};

class IOBufMessageReader : public capnp::MessageReader {
 public:
  IOBufMessageReader(std::unique_ptr<IOBuf>&& buf,
                     capnp::ReaderOptions options = capnp::ReaderOptions())
      : capnp::MessageReader(options), buf_(std::move(buf)),
        dp_(buf_->GetDataPointer()) {
    auto nseg = dp_.Get<uint32_t>() + 1;
    uint32_t seg_sizes[nseg];  // NOLINT
    for (uint32_t i = 0; i < nseg; ++i) {
      seg_sizes[i] = dp_.Get<uint32_t>();
    }
    if (nseg % 2 == 0)
      dp_.Advance(4);

    segments_.reserve(nseg);
    for (uint32_t i = 0; i < nseg; ++i) {
      auto size = seg_sizes[i];
      auto data = dp_.Get(size * sizeof(capnp::word));
      segments_.emplace_back(reinterpret_cast<const capnp::word*>(data), size);
    }
  }

  kj::ArrayPtr<const capnp::word> getSegment(uint id) override {
    return segments_[id];
  }

  std::unique_ptr<IOBuf> GetBuf() { return std::move(buf_); }

 private:
  std::unique_ptr<IOBuf> buf_;
  std::vector<kj::ArrayPtr<const capnp::word>> segments_;
  // The lifetime of the pointers in segments_ must be less than the lifetime of
  // this  datapointer, so we store it as a member.
  IOBuf::DataPointer dp_;
};

std::unique_ptr<IOBuf> AppendHeader(IOBufMessageBuilder& builder);
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_CAPNPMESSAGE_H_
