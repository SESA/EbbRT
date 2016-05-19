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
#include <ebbrt/TypeTraits.h>

namespace ebbrt {

template <class T> class EbbRef {
 public:
  constexpr explicit EbbRef(EbbId id = 0)
      : ref_(trans::kVMemStart + sizeof(LocalEntry) * id) {}

  template <typename From>
  EbbRef(const EbbRef<From>& from,
         typename std::enable_if_t<std::is_convertible<From*, T*>::value>* =
             nullptr)
      : ref_(from.ref_) {}

  template <typename From>
  explicit EbbRef(
      const EbbRef<From>& from,
      typename std::enable_if_t<is_explicitly_convertible<From*, T*>::value &&
                                !std::is_convertible<From*, T*>::value>* =
          nullptr)
      : ref_(from.ref_) {}

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

  explicit operator EbbId() const {
    return (ref_ - trans::kVMemStart) / sizeof(LocalEntry);
  }

 private:
  template <typename U> friend class EbbRef;
  uintptr_t ref_;
};
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_EBBREF_H_
