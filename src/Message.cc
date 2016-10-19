//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Message.h"

#include <unordered_map>

std::unordered_map<uint64_t, ebbrt::MessagableBase& (*)(ebbrt::EbbId),
                   ebbrt::Hasher>
    ebbrt::fault_map __attribute__((init_priority(101)));

ebbrt::MessagableBase& ebbrt::GetMessagableRef(EbbId id, uint64_t type_code) {
  auto local_entry = GetLocalEntry(id);
  if (local_entry.ref == nullptr) {
    auto it = fault_map.find(type_code);
    if (it == fault_map.end())
      throw std::runtime_error("GetMessagableRef missing fault for type code");
    return it->second(id);
  } else {
    return *static_cast<MessagableBase*>(local_entry.ref);
  }
}
