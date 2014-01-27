//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_STATICSHAREDEBB_H_
#define COMMON_SRC_INCLUDE_EBBRT_STATICSHAREDEBB_H_

#include <ebbrt/EbbRef.h>

namespace ebbrt {
template <class T> class StaticSharedEbb {
 public:
  static void Init() {}
  static T &HandleFault(EbbId id) {
    static T rep;
    EbbRef<T>::CacheRef(id, rep);
    return rep;
  }
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_STATICSHAREDEBB_H_
