//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/LocalIdMap.h>

bool ebbrt::LocalIdMap::Insert(const ValueType& value) {
  return map_.insert(value);
}

bool ebbrt::LocalIdMap::Insert(Accessor& result, const EbbId& key) {
  return map_.insert(result, key);
}

bool ebbrt::LocalIdMap::Find(ConstAccessor& result, const EbbId& key) const {
  return map_.find(result, key);
}

bool ebbrt::LocalIdMap::Find(Accessor& result, const EbbId& key) {
  return map_.find(result, key);
}
