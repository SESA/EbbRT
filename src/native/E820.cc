//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "E820.h"

#include <cinttypes>

#include "../ExplicitlyConstructed.h"
#include "Debug.h"

ebbrt::ExplicitlyConstructed<boost::container::static_vector<
    ebbrt::e820::Entry, ebbrt::e820::kMaxEntries>>
    ebbrt::e820::map;

void ebbrt::e820::Init(multiboot::Information* mbi) {
  map.construct();
  multiboot::MemoryRegion::ForEachRegion(
      reinterpret_cast<const void*>(mbi->memory_map_address_),
      mbi->memory_map_length_, [](const multiboot::MemoryRegion& mmregion) {
        map->emplace_back(mmregion.base_address_, mmregion.length_,
                          mmregion.type_);
      });

  std::sort(std::begin(*map), std::end(*map));
}

void ebbrt::e820::PrintMap() {
  kprintf("e820::map:\n");
  std::for_each(std::begin(*map), std::end(*map), [](const Entry& entry) {
    kprintf("%#018" PRIx64 "-%#018" PRIx64 " ", entry.addr(),
            entry.addr() + entry.length() - 1);
    switch (entry.type()) {
    case Entry::kTypeRam:
      kprintf("usable");
      break;
    case Entry::kTypeReservedUnusable:
      kprintf("reserved");
      break;
    case Entry::kTypeAcpiReclaimable:
      kprintf("ACPI data");
      break;
    case Entry::kTypeAcpiNvs:
      kprintf("ACPI NVS");
      break;
    case Entry::kTypeBadMemory:
      kprintf("unusable");
      break;
    default:
      kprintf("type %" PRIu32, entry.type());
      break;
    }
    kprintf("\n");
  });
}
