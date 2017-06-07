//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_HASH_H_
#define COMMON_SRC_INCLUDE_EBBRT_HASH_H_

#include <stdexcept>

namespace ebbrt {
namespace hash {

// string literal
class conststr {
 public:
  template <std::size_t N>
  constexpr conststr(const char (&a)[N]) : p(a), sz(N - 1) {}

  constexpr char operator[](std::size_t n) const {
    return n < sz ? p[n] : throw std::out_of_range("");
  }
  constexpr std::size_t size() const { return sz; }

 private:
  const char* p;
  std::size_t sz;
};

/*  Jenkin's one-at-a-time Hash
 *  https://en.wikipedia.org/wiki/Jenkins_hash_function */
constexpr uint32_t rsx(uint32_t hash, uint8_t rshift) {
  return hash ^ (hash >> rshift);
}
constexpr uint32_t lsa(uint32_t hash, uint8_t lshift) {
  return hash + ((hash << lshift) & 0xFFFFFFFF);
}
constexpr uint32_t oaat_loop(conststr s, uint32_t hash, std::uint32_t len) {
  return len == 0 ? hash
                  : oaat_loop(s, rsx(lsa(((hash << 1) + s[(len - 1)]), 10), 6),
                              len - 1);
}
constexpr uint32_t one_at_a_time_hash(conststr s, uint32_t seed) {
  return lsa(rsx(lsa(oaat_loop(s, seed, s.size()), 3), 11), 15);
}
}

using hash::conststr;
using hash::one_at_a_time_hash;

constexpr uint32_t static_string_hash(conststr s, uint32_t seed = 0) {
  return one_at_a_time_hash(s, seed);
}
}

#endif  // COMMON_SRC_INCLUDE_EBBRT_HASH_H_
