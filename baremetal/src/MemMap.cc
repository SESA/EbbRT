//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/MemMap.h>

#include <ebbrt/Align.h>
#include <ebbrt/EarlyPageAllocator.h>
#include <ebbrt/Numa.h>

std::array<ebbrt::mem_map::Section, ebbrt::mem_map::kSections>
ebbrt::mem_map::sections;

void ebbrt::mem_map::Init() {
  for (const auto &node : numa::nodes) {
    for (const auto &memblock : node.memblocks) {
      auto nid = memblock.nid;
      auto start = memblock.start;
      auto end = memblock.end;

      start = Pfn(start.val() & kPageSectionMask);
      for (auto pfn = start; pfn < end; pfn += kPagesPerSection) {
        auto &section = PfnToSection(pfn);
        if (!section.Present()) {
          section.SetEarlyNid(nid);
        }
      }
    }
  }

  const auto section_map_bytes = kPagesPerSection * sizeof(Page);
  const auto section_map_pages =
      align::Up(section_map_bytes, pmem::kPageSize) >> pmem::kPageShift;
  for (unsigned i = 0; i < sections.size(); ++i) {
    auto &section = sections[i];
    if (section.Present()) {
      auto nid = section.EarlyNid();
      auto map_pfn = early_page_allocator::AllocatePage(section_map_pages, nid);
      auto map_addr = map_pfn.ToAddr();
      auto map = reinterpret_cast<Page *>(map_addr);
      for (unsigned j = 0; j < kPagesPerSection; j++) {
        new (reinterpret_cast<void *>(&map[j])) Page(nid);
      }
      section.SetMap(map_addr, i);
    }
  }
}
