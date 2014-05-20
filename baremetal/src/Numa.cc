//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Numa.h>

#include <boost/utility.hpp>

#include <ebbrt/Cpu.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EarlyPageAllocator.h>
#include <ebbrt/ExplicitlyConstructed.h>

ebbrt::ExplicitlyConstructed<boost::container::static_vector<
    ebbrt::numa::Node, ebbrt::numa::kMaxNodes>> ebbrt::numa::nodes;

namespace {
const constexpr size_t kMaxLocalApic = 256;
const constexpr size_t kMaxPxmDomains = 256;

std::array<ebbrt::Nid, kMaxLocalApic> apic_to_node_map;
std::array<ebbrt::Nid, kMaxPxmDomains> pxm_to_node_map;
std::array<int32_t, ebbrt::numa::kMaxNodes> node_to_pxm_map;
}

void ebbrt::numa::Init() {
  for (auto& numa_node : *nodes) {
    std::sort(numa_node.memblocks.begin(), numa_node.memblocks.end());
    for (auto& memblock : numa_node.memblocks) {
      early_page_allocator::SetNidRange(memblock.start, memblock.end,
                                        memblock.nid);
    }
    if (numa_node.memblocks.empty()) {
      numa_node.pfn_start = Pfn::None();
      numa_node.pfn_end = Pfn::None();
    } else {
      numa_node.pfn_start = numa_node.memblocks.begin()->start;
      numa_node.pfn_end = boost::prior(numa_node.memblocks.end())->end;
    }
  }
}

void ebbrt::numa::EarlyInit() {
  nodes.construct();
  std::fill(apic_to_node_map.begin(), apic_to_node_map.end(), Nid::None());
  std::fill(pxm_to_node_map.begin(), pxm_to_node_map.end(), Nid::None());
  std::fill(node_to_pxm_map.begin(), node_to_pxm_map.end(), -1);
}

ebbrt::Nid ebbrt::numa::SetupNode(size_t proximity_domain) {
  kbugon(proximity_domain >= kMaxPxmDomains,
         "Proximity domain exceeds MAX_PXM_DOMAINS");
  auto node = pxm_to_node_map[proximity_domain];
  if (node == Nid::None()) {
    nodes->emplace_back();
    node = Nid(nodes->size() - 1);
    pxm_to_node_map[proximity_domain] = node;
    node_to_pxm_map[node.val()] = proximity_domain;
  }
  return node;
}

void ebbrt::numa::MapApicToNode(size_t apic_id, Nid nid) {
  auto p = Cpu::GetByApicId(apic_id);
  kbugon(p == nullptr, "Asked to map non existent apic!");
  p->set_nid(nid);
}

void ebbrt::numa::AddMemBlock(ebbrt::Nid nid, ebbrt::Pfn start,
                              ebbrt::Pfn end) {
  (*nodes)[nid.val()].memblocks.emplace_back(start, end, nid);
}
