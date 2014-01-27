//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_E820_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_E820_H_

#include <algorithm>
#include <cstdint>

#include <boost/container/static_vector.hpp>

#include <ebbrt/Multiboot.h>

namespace ebbrt {
namespace e820 {

class Entry {
 public:
  static const constexpr uint32_t kTypeRam = 1;
  static const constexpr uint32_t kTypeReservedUnusable = 2;
  static const constexpr uint32_t kTypeAcpiReclaimable = 3;
  static const constexpr uint32_t kTypeAcpiNvs = 4;
  static const constexpr uint32_t kTypeBadMemory = 5;

  Entry(uint64_t addr, uint64_t length, uint32_t type)
      : addr_(addr), length_(length), type_(type) {}

  void TrimBelow(uint64_t new_addr) {
    if (addr_ > new_addr || addr_ + length_ < new_addr)
      return;

    length_ -= new_addr - addr_;
    addr_ = new_addr;
  }

  void TrimAbove(uint64_t new_addr) {
    if (addr_ > new_addr || addr_ + length_ < new_addr)
      return;
    length_ -= (addr_ + length_) - new_addr;
  }

  uint64_t addr() const { return addr_; }
  uint64_t length() const { return length_; }
  uint32_t type() const { return type_; }

 private:
  uint64_t addr_;
  uint64_t length_;
  uint32_t type_;
};

inline bool operator==(const Entry& lhs, const Entry& rhs) {
  return lhs.addr() == rhs.addr();
}

inline bool operator!=(const Entry& lhs, const Entry& rhs) {
  return !operator==(lhs, rhs);
}

inline bool operator<(const Entry& lhs, const Entry& rhs) {
  return lhs.addr() < rhs.addr();
}

inline bool operator>(const Entry& lhs, const Entry& rhs) {
  return operator<(rhs, lhs);
}

inline bool operator<=(const Entry& lhs, const Entry& rhs) {
  return !operator>(lhs, rhs);
}

inline bool operator>=(const Entry& lhs, const Entry& rhs) {
  return !operator<(lhs, rhs);
}

const constexpr size_t kMaxEntries = 128;
extern boost::container::static_vector<Entry, kMaxEntries> map;

void Init(MultibootInformation* mbi);

void PrintMap();

void Reserve(uint64_t addr, uint64_t length);

template <typename F> void ForEachUsableRegion(F f) {
  std::for_each(std::begin(map),
                std::end(map),
                [ = ](const Entry & entry) {
    if (entry.type() == Entry::kTypeRam) {
      f(entry);
    }
  });
}

}  // namespace e820
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_E820_H_
