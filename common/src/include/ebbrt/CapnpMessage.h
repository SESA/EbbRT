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
class BufferMessageBuilder : public capnp::MessageBuilder {
 public:
  kj::ArrayPtr<capnp::word> allocateSegment(capnp::uint minimumSize) override {
    capnp::uint size = minimumSize;

    void* result = std::calloc(size, sizeof(capnp::word));
    if (result == nullptr) {
      throw std::bad_alloc();
    }

    if (buf_.data() == nullptr) {
      buf_ = Buffer(result, size * sizeof(capnp::word),
                    [](void* p, size_t sz) { std::free(p); });
    } else {
      buf_.emplace_back(result, size * sizeof(capnp::word),
                        [](void* p, size_t sz) { std::free(p); });
    }

    return kj::arrayPtr(reinterpret_cast<capnp::word*>(result), size);
  }

  Buffer GetBuffer() { return std::move(buf_); }

 private:
  Buffer buf_;
};

class BufferMessageReader : public capnp::MessageReader {
 public:
  BufferMessageReader(Buffer buf,
                      capnp::ReaderOptions options = capnp::ReaderOptions())
      : capnp::MessageReader(options), buf_(std::move(buf)) {
    auto p = buf.GetDataPointer();
    auto nseg = p.Get<uint32_t>();
    auto p2 = buf.GetDataPointer();
    p2.Advance(align::Up(4 + 4 * nseg, 8));
    segments_.reserve(nseg);
    for (uint32_t i = 0; i < nseg; ++i) {
      auto size = p.Get<uint32_t>();
      segments_.emplace_back(static_cast<capnp::word*>(p2.Addr()), size);
      p2.Advance(size * sizeof(capnp::word));
    }
  }

  virtual kj::ArrayPtr<const capnp::word> getSegment(uint id) override {
    return segments_[id];
  }

 private:
  Buffer buf_;
  std::vector<kj::ArrayPtr<const capnp::word>> segments_;
};

Buffer AppendHeader(BufferMessageBuilder builder);
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_CAPNPMESSAGE_H_
