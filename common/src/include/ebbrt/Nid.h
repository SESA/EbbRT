//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_NID_H_
#define COMMON_SRC_INCLUDE_EBBRT_NID_H_

namespace ebbrt {
class Nid {
 public:
  constexpr Nid() : val_(-1) {}
  explicit constexpr Nid(int val) : val_(val) {}

  static constexpr Nid None() { return Nid(-1); }
  static constexpr Nid Any() { return Nid(-2); }

  int val() const noexcept { return val_; }

 private:
  int val_;
};

inline bool operator==(const Nid& lhs, const Nid& rhs) {
  return lhs.val() == rhs.val();
}

inline bool operator!=(const Nid& lhs, const Nid& rhs) {
  return !operator==(lhs, rhs);
}

inline bool operator<(const Nid& lhs, const Nid& rhs) {
  return lhs.val() < rhs.val();
}

inline bool operator>(const Nid& lhs, const Nid& rhs) {
  return operator<(rhs, lhs);
}

inline bool operator<=(const Nid& lhs, const Nid& rhs) {
  return !operator>(lhs, rhs);
}

inline bool operator>=(const Nid& lhs, const Nid& rhs) {
  return !operator<(lhs, rhs);
}

inline size_t operator-(const Nid& lhs, const Nid& rhs) {
  return lhs.val() - rhs.val();
}

}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_NID_H_
