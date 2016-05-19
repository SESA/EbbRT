//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_TYPETRAITS_H_
#define COMMON_SRC_INCLUDE_EBBRT_TYPETRAITS_H_

namespace ebbrt {

template <typename From, typename To> struct is_explicitly_convertible {
  template <typename T> static void f(T);

  template <typename F, typename T>
  static constexpr auto test(int)  // NOLINT
      -> decltype(f(static_cast<T>(std::declval<F>())), true) {
    return true;
  }

  template <typename F, typename T> static constexpr auto test(...) -> bool {
    return false;
  }

  static bool const value = test<From, To>(0);
};

}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_TYPETRAITS_H_
