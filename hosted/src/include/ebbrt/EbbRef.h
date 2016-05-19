//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_EBBREF_H_
#define HOSTED_SRC_INCLUDE_EBBRT_EBBREF_H_

#include <ebbrt/Context.h>
#include <ebbrt/EbbId.h>
#include <ebbrt/LocalEntry.h>
#include <ebbrt/TypeTraits.h>
#include <type_traits>

namespace ebbrt {
template <class T> class EbbRef {
 public:
  constexpr explicit EbbRef(EbbId id = 0) : ebbid_(id) {}

  template <typename From>
  EbbRef(const EbbRef<From>& from,
         typename std::enable_if_t<std::is_convertible<From*, T*>::value>* =
             nullptr)
      : ebbid_(from.ebbid_) {}

  template <typename From>
  explicit EbbRef(
      const EbbRef<From>& from,
      typename std::enable_if_t<is_explicitly_convertible<From*, T*>::value &&
                                !std::is_convertible<From*, T*>::value>* =
          nullptr)
      : ebbid_(from.ebbid_) {}

  T* operator->() const { return &operator*(); }

  T& operator*() const {
    if (active_context == nullptr) {
      throw std::runtime_error("Cannot invoke Ebb without active context");
    }
    auto local_entry = active_context->GetLocalEntry(ebbid_);
    auto lref = static_cast<T*>(local_entry.ref);
    if (lref == nullptr) {
      lref = &(T::HandleFault(ebbid_));
    }
    return *lref;
  }

  static void CacheRef(EbbId id, T& ref) {
    LocalEntry le;
    le.ref = static_cast<void*>(&ref);
    active_context->SetLocalEntry(id, le);
  }

  explicit operator EbbId() const { return ebbid_; }

 private:
  template <typename U> friend class EbbRef;
  EbbId ebbid_;
};

}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_EBBREF_H_
