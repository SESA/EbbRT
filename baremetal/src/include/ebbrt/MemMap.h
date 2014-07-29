//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_MEMMAP_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_MEMMAP_H_

#include <array>
#include <cstdint>
#include <type_traits>

#include <boost/intrusive/list.hpp>

#include <ebbrt/Debug.h>
#include <ebbrt/ExplicitlyConstructed.h>
#include <ebbrt/Numa.h>
#include <ebbrt/Pfn.h>
#include <ebbrt/PMem.h>
#include <ebbrt/SlabObject.h>

namespace ebbrt {
struct SlabCache;

namespace mem_map {
const constexpr size_t kMaxPhysMemBits = 46;
const constexpr size_t kSectionSizeBits = 27;
const constexpr size_t kPfnSectionShift = kSectionSizeBits - pmem::kPageShift;
const constexpr size_t kPagesPerSection = 1 << kPfnSectionShift;
const constexpr size_t kPageSectionMask = (~(kPagesPerSection - 1));
const constexpr size_t kSectionsShift = kMaxPhysMemBits - kSectionSizeBits;
const constexpr size_t kSections = 1 << kSectionsShift;

struct Page {
  enum class Usage : uint8_t {
    kReserved,
    kPageAllocator,
    kSlabAllocator,
    kInUse
  } usage;

  explicit Page(Nid nid) : usage(Usage::kReserved), nid(nid.val()) {}

  static_assert(numa::kMaxNodes <= 256,
                "Not enough space to store nid in struct page");
  uint8_t nid;
  union Data {
    uint8_t order;
    struct SlabData {
      // In slab allocator
      ExplicitlyConstructed<boost::intrusive::list_member_hook<
          boost::intrusive::link_mode<boost::intrusive::normal_link>>>
      member_hook;
      ExplicitlyConstructed<CompactFreeObjectList> list;
      SlabCache* cache;
      size_t used;
    } __attribute__((packed)) slab_data;
  } __attribute__((packed)) data;
} __attribute__((packed));

void Init();

class Section {
 public:
  bool Present() const { return mem_map_ & kPresent; }
  bool Valid() const { return mem_map_ & kMapMask; }
  Page* GetPage(Pfn pfn) const {
    // mem_map encodes the starting frame so we can just index off it
    // e.g.: mem_map_ = mem_map_address - start_pfn
    return MapAddr() + pfn.val();
  }

 private:
  static const constexpr uintptr_t kPresent = 1;
  static const constexpr uintptr_t kHasMemMap = 2;
  static const constexpr uintptr_t kMapMask = ~3;
  static const constexpr size_t kNidShift = 2;

  Page* MapAddr() const { return reinterpret_cast<Page*>(mem_map_ & kMapMask); }
  void SetEarlyNid(Nid nid) { mem_map_ = (nid.val() << kNidShift) | kPresent; }
  Nid EarlyNid() { return Nid(mem_map_ >> kNidShift); }
  void SetMap(uintptr_t map_addr, size_t index) {
    // set the addr to take into account the starting frame so get_page is more
    // efficient
    auto addr = map_addr - index * kPagesPerSection * sizeof(Page);
    mem_map_ = addr | kPresent | kHasMemMap;
  }

  uintptr_t mem_map_;

  friend void ::ebbrt::mem_map::Init();
};

extern std::array<Section, kSections> sections;

inline size_t PfnToSectionIndex(Pfn pfn) {
  return pfn.val() >> kPfnSectionShift;
}

inline Section& PfnToSection(Pfn pfn) {
  return sections[PfnToSectionIndex(pfn)];
}

inline Page* PfnToPage(Pfn pfn) {
  auto section = PfnToSection(pfn);
  if (!section.Valid() || !section.Present()) {
    return nullptr;
  }
  return section.GetPage(pfn);
}

inline Page* AddrToPage(uintptr_t addr) { return PfnToPage(Pfn::Down(addr)); }

inline Page* AddrToPage(void* addr) {
  return AddrToPage(reinterpret_cast<uintptr_t>(addr));
}
}  // namespace mem_map
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_MEMMAP_H_
