//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_VMEM_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_VMEM_H_

#include <algorithm>

#include <ebbrt/PMem.h>
#include <ebbrt/Pfn.h>

namespace ebbrt {
namespace vmem {
class Pte {
 public:
  static const constexpr uint32_t kPermRead = 1 << 0;
  static const constexpr uint32_t kPermWrite = 1 << 1;
  static const constexpr uint32_t kPermExec = 1 << 2;

  Pte() { Clear(); }
  explicit Pte(uint64_t raw) : raw_(raw) {}
  bool Present() const { return raw_ & 1; }
  uint64_t Addr(bool large) const {
    auto ret = raw_ & ((UINT64_C(1) << 52) - 1);
    ret &= ~((UINT64_C(1) << 12) - 1);
    ret &= ~(static_cast<uint64_t>(large) << 12);  // ignore PAT bit
    return ret;
  }
  bool Large() const { return raw_ & (1 << 7); }
  void Clear() { raw_ = 0; }
  void SetPresent(bool val) { SetBit(0, val); }
  void SetWritable(bool val) { SetBit(1, val); }
  void SetLarge(bool val) { SetBit(7, val); }
  void SetNx(bool val) { SetBit(63, val); }
  void SetAddr(uint64_t addr, bool large) {
    auto mask = (0x8000000000000fff | static_cast<uint64_t>(large) << 12);
    raw_ = (raw_ & mask) | addr;
  }
  void Set(uint64_t phys_addr, bool large,
           unsigned permissions = kPermRead | kPermWrite | kPermExec) {
    raw_ = 0;
    SetPresent(permissions != 0);
    SetWritable(permissions & kPermWrite);
    SetLarge(large);
    SetAddr(phys_addr, large);
    SetNx(!(permissions & kPermExec));
  }
  void SetNormal(uint64_t phys_addr,
                 uint32_t permissions = kPermRead | kPermWrite | kPermExec) {
    Set(phys_addr, false, permissions);
  }
  void SetLarge(uint64_t phys_addr,
                uint32_t permissions = kPermRead | kPermWrite | kPermExec) {
    Set(phys_addr, true, permissions);
  }

 private:
  void SetBit(size_t bit_num, bool value) {
    raw_ &= ~(UINT64_C(1) << bit_num);
    raw_ |= static_cast<uint64_t>(value) << bit_num;
  }

  uint64_t raw_;
};

// TODO(dschatz): support 1g pages
const constexpr size_t kNumPageSizes = 2;

inline size_t PtIndex(uintptr_t virt_addr, size_t level) {
  return (virt_addr >> (12 + level * 9)) & ((1 << 9) - 1);
}

inline uint64_t Canonical(uint64_t addr) {
  // sign extend for x86 canonical address
  return (int64_t(addr) << 16) >> 16;
}

template <typename Found_Entry_Func, typename Empty_Entry_Func>
void TraversePageTable(Pte& entry, uint64_t virt_start, uint64_t virt_end,
                       uint64_t base_virt, size_t level, Found_Entry_Func found,
                       Empty_Entry_Func empty) {
  if (!entry.Present())
    if (!empty(entry))
      return;

  --level;
  auto pt = reinterpret_cast<Pte*>(entry.Addr(false));
  auto step = UINT64_C(1) << (12 + level * 9);
  auto idx_begin = PtIndex(std::max(virt_start, base_virt), level);
  auto base_virt_end = Canonical(base_virt + step * 512 - 1);
  auto idx_end = PtIndex(std::min(virt_end - 1, base_virt_end), level);
  base_virt += Canonical(idx_begin * step);
  for (size_t idx = idx_begin; idx <= idx_end; ++idx) {
    if (level < kNumPageSizes && virt_start <= base_virt &&
        virt_end >= base_virt + step) {
      found(pt[idx], base_virt, level);
    } else {
      TraversePageTable(pt[idx], virt_start, virt_end, base_virt, level, found,
                        empty);
    }
    base_virt = Canonical(base_virt + step);
  }
}

void Init();
void EnableRuntimePageTable();
void EarlyMapMemory(uint64_t addr, uint64_t length);
void EarlyUnmapMemory(uint64_t addr, uint64_t length);
void MapMemory(Pfn vfn, Pfn pfn, uint64_t length = pmem::kPageSize);
void MapMemoryLarge(uintptr_t vaddr, Pfn pfn,
                    uint64_t length = pmem::kLargePageSize);
void ApInit(size_t index);

Pte& GetPageTableRoot();

}  // namespace vmem
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_VMEM_H_
