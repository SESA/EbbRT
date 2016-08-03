//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_PFN_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_PFN_H_

#include <ebbrt/PMem.h>

namespace ebbrt {
class Pfn {
 public:
  constexpr Pfn() : val_(UINTPTR_MAX) {}
  explicit constexpr Pfn(uintptr_t val) : val_(val) {}

  static constexpr Pfn None() { return Pfn(); }

  static Pfn Up(uintptr_t addr) {
    auto pfn = (addr + pmem::kPageSize - 1) >> pmem::kPageShift;
    return Pfn(pfn);
  }

  static Pfn Down(uintptr_t addr) {
    auto pfn = addr >> pmem::kPageShift;
    return Pfn(pfn);
  }

  static Pfn LDown(uintptr_t addr) {
    auto pfn = addr >> pmem::kLargePageShift;
    return Pfn(pfn);
  }

  static Pfn Up(void* addr) { return Up(reinterpret_cast<uintptr_t>(addr)); }

  static Pfn Down(void* addr) {
    return Down(reinterpret_cast<uintptr_t>(addr));
  }

  static Pfn LDown(void* addr) {
    return LDown(reinterpret_cast<uintptr_t>(addr));
  }

  uintptr_t ToLAddr() const { return val_ << pmem::kLargePageShift; }
  uintptr_t ToAddr() const { return val_ << pmem::kPageShift; }
  uintptr_t val() const { return val_; }

  template <typename T,
            class = typename std::enable_if<std::is_integral<T>::value>::type>
  Pfn& operator+=(const T& rhs) {
    val_ += rhs;
    return *this;
  }

  template <typename T,
            class = typename std::enable_if<std::is_integral<T>::value>::type>
  Pfn& operator-=(const T& rhs) {
    val_ -= rhs;
    return *this;
  }

 private:
  uintptr_t val_;
};

inline bool operator==(const Pfn& lhs, const Pfn& rhs) {
  return lhs.val() == rhs.val();
}

inline bool operator!=(const Pfn& lhs, const Pfn& rhs) {
  return !operator==(lhs, rhs);
}

inline bool operator<(const Pfn& lhs, const Pfn& rhs) {
  return lhs.val() < rhs.val();
}

inline bool operator>(const Pfn& lhs, const Pfn& rhs) {
  return operator<(rhs, lhs);
}

inline bool operator<=(const Pfn& lhs, const Pfn& rhs) {
  return !operator>(lhs, rhs);
}

inline bool operator>=(const Pfn& lhs, const Pfn& rhs) {
  return !operator<(lhs, rhs);
}

inline size_t operator-(const Pfn& lhs, const Pfn& rhs) {
  return lhs.val() - rhs.val();
}

template <typename T,
          class = typename std::enable_if<std::is_integral<T>::value>::type>
Pfn operator+(Pfn lhs, T npages) {
  lhs += npages;
  return lhs;
}

template <typename T,
          class = typename std::enable_if<std::is_integral<T>::value>::type>
Pfn operator-(Pfn lhs, T npages) {
  lhs -= npages;
  return lhs;
}
}  // namespace ebbrt

namespace std {
template <> struct hash<ebbrt::Pfn> {
  size_t operator()(const ebbrt::Pfn& x) const {
    return hash<uintptr_t>()(x.val());
  }
};
}  // namespace std

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_PFN_H_
