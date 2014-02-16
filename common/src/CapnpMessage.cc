//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/CapnpMessage.h>

struct Header {
  uint32_t num_segments;
  uint32_t segment_sizes[0];
};

ebbrt::Buffer ebbrt::AppendHeader(ebbrt::BufferMessageBuilder& builder) {
  auto segs = builder.getSegmentsForOutput();
  auto nsegs = segs.size();
  auto header_size = align::Up(4 + 4 * nsegs, 8);
  auto header_buf = Buffer(header_size, true);
  auto h = static_cast<Header*>(header_buf.data());
  h->num_segments = nsegs;
  unsigned i = 0;
  for (auto& seg : segs) {
    h->segment_sizes[i] = seg.size();
    ++i;
  }
  header_buf.emplace_back(std::move(builder.GetBuffer()));
  return header_buf;
}
