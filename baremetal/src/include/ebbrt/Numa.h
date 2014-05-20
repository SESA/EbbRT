//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NUMA_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NUMA_H_

#include <boost/container/static_vector.hpp>

#include <ebbrt/ExplicitlyConstructed.h>
#include <ebbrt/Nid.h>
#include <ebbrt/Pfn.h>

namespace ebbrt {
namespace numa {
const constexpr size_t kMaxNodes = 256;
const constexpr size_t kMaxMemblocks = 256;

struct Memblock {
  Memblock(Pfn start, Pfn end, Nid nid) : start(start), end(end), nid(nid) {}
  Pfn start;
  Pfn end;
  Nid nid;
};

inline bool operator<(const Memblock& lhs, const Memblock& rhs) noexcept {
  return lhs.start < rhs.start;
}

struct Node {
  boost::container::static_vector<Memblock, kMaxMemblocks> memblocks;
  Pfn pfn_start;
  Pfn pfn_end;
};

void EarlyInit();
void Init();
Nid SetupNode(size_t proximity_domain);
void MapApicToNode(size_t apic_id, Nid nid);
void AddMemBlock(Nid nid, Pfn start, Pfn End);

extern ebbrt::ExplicitlyConstructed<
    boost::container::static_vector<Node, kMaxNodes>> nodes;

}  // namespace numa
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NUMA_H_
