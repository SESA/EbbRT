//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_EBBREF_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_EBBREF_H_

#include <ebbrt/Compiler.h>
#include <ebbrt/EbbId.h>
#include <ebbrt/LocalEntry.h>
#include <ebbrt/Trans.h>

namespace ebbrt {
template <class T> class EbbRef {
 public:
  constexpr EbbRef() : ref_(0) {}

  constexpr explicit EbbRef(EbbId id)
      : ref_(trans::kVMemStart + sizeof(LocalEntry) * id) {}

  T* operator->() const { return &operator*(); }

  T& operator*() const {
    auto lref = *reinterpret_cast<T**>(ref_);
    if (unlikely(lref == nullptr)) {
      auto id = (ref_ - trans::kVMemStart) / sizeof(LocalEntry);
      lref = &(T::HandleFault(id));
    }
    return *lref;
  }

  static void CacheRef(EbbId id, T& ref) {
    auto le = reinterpret_cast<LocalEntry*>(trans::kVMemStart +
                                            sizeof(LocalEntry) * id);
    le->ref = &ref;
  }

  operator EbbId() const {
    return (ref_ - trans::kVMemStart) / sizeof(LocalEntry);
  }

 private:
  uintptr_t ref_;
};
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_EBBREF_H_
