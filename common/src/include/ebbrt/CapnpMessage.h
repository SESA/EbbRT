//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_CAPNPMESSAGE_H_
#define COMMON_SRC_INCLUDE_EBBRT_CAPNPMESSAGE_H_

#include <cstdlib>
#include <vector>

#include <capnp/message.h>

#include <ebbrt/Align.h>
#include <ebbrt/Buffer.h>

namespace ebbrt {
class IOBufMessageBuilder : public capnp::MessageBuilder {
 public:
  kj::ArrayPtr<capnp::word> allocateSegment(capnp::uint minimumSize) override {
    auto buf = IOBuf::Create(minimumSize * sizeof(capnp::word), true);
    auto ptr = buf->WritableData();

    if (buf_ == nullptr) {
      buf_ = std::move(buf);
    } else {
      buf_->Prev()->AppendChain(std::move(buf));
    }

    return kj::arrayPtr(reinterpret_cast<capnp::word*>(ptr), minimumSize);
  }

  std::unique_ptr<IOBuf> Move() { return std::move(buf_); }

 private:
  std::unique_ptr<IOBuf> buf_;
};

class IOBufMessageReader : public capnp::MessageReader {
 public:
  IOBufMessageReader(std::unique_ptr<IOBuf>&& buf,
                     capnp::ReaderOptions options = capnp::ReaderOptions())
      : capnp::MessageReader(options), buf_(std::move(buf)) {
    auto p = buf_->GetDataPointer();
    auto nseg = p.Get<uint32_t>();
    auto p2 = buf_->GetDataPointer();
    p2.Advance(align::Up(4 + 4 * nseg, 8));
    segments_.reserve(nseg);
    for (uint32_t i = 0; i < nseg; ++i) {
      auto size = p.Get<uint32_t>();
      segments_.emplace_back(reinterpret_cast<const capnp::word*>(p2.Data()),
                             size);
      p2.Advance(size * sizeof(capnp::word));
    }
  }

  virtual kj::ArrayPtr<const capnp::word> getSegment(uint id) override {
    return segments_[id];
  }

  std::unique_ptr<IOBuf> GetBuf() { return std::move(buf_); }

 private:
  std::unique_ptr<IOBuf> buf_;
  std::vector<kj::ArrayPtr<const capnp::word>> segments_;
};

std::unique_ptr<IOBuf> AppendHeader(IOBufMessageBuilder& builder);
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_CAPNPMESSAGE_H_
