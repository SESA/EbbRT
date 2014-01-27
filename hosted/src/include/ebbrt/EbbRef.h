//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_EBBREF_H_
#define HOSTED_SRC_INCLUDE_EBBRT_EBBREF_H_

#include <ebbrt/EbbId.h>
#include <ebbrt/LocalEntry.h>
#include <ebbrt/Context.h>

namespace ebbrt {
template <class T> class EbbRef {
 public:
  constexpr explicit EbbRef(EbbId id) : ebbid_(id) {}

  T *operator->() const {
    if (active_context == nullptr) {
      throw std::runtime_error("Cannot invoke Ebb without active context");
    }
    auto local_entry = active_context->GetLocalEntry(ebbid_);
    auto lref = static_cast<T *>(local_entry.ref);
    if (lref == nullptr) {
      lref = &(T::HandleFault(ebbid_));
    }
    return lref;
  }

  static void CacheRef(EbbId id, T &ref) {
    LocalEntry le;
    le.ref = static_cast<void *>(&ref);
    active_context->SetLocalEntry(id, le);
  }

 private:
  EbbId ebbid_;
};

}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_EBBREF_H_
