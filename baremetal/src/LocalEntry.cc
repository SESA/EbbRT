//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/LocalEntry.h>

#include <ebbrt/Trans.h>

ebbrt::LocalEntry ebbrt::GetLocalEntry(EbbId id) {
  auto lref = reinterpret_cast<LocalEntry*>(trans::kVMemStart +
                                            sizeof(LocalEntry) * id);
  return *lref;
}

void ebbrt::SetLocalEntry(EbbId id, LocalEntry le) {
  auto lref = reinterpret_cast<LocalEntry*>(trans::kVMemStart +
                                            sizeof(LocalEntry) * id);
  *lref = le;
}
