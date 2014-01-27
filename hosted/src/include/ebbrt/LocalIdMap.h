//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_LOCALIDMAP_H_
#define HOSTED_SRC_INCLUDE_EBBRT_LOCALIDMAP_H_

#include <memory>
#include <utility>

#include <boost/any.hpp>
#include <tbb/concurrent_hash_map.h>

#include <ebbrt/CacheAligned.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {
class LocalIdMap : public StaticSharedEbb<LocalIdMap>, public CacheAligned {
 public:
  typedef tbb::concurrent_hash_map<
      EbbId,
      boost::any,
      tbb::tbb_hash_compare<EbbId>,
      std::allocator<std::pair<const EbbId, boost::any> > > MapType;

  typedef MapType::const_accessor ConstAccessor;
  typedef MapType::accessor Accessor;
  typedef MapType::value_type ValueType;

  bool Find(ConstAccessor& result, const EbbId& key) const;
  bool Find(Accessor& result, const EbbId& key);
  bool Insert(ConstAccessor& result, const EbbId& key);
  bool Insert(Accessor& result, const EbbId& key);
  bool Insert(ConstAccessor& result, const ValueType& value);
  bool Insert(Accessor& result, const ValueType& value);
  bool Insert(const ValueType& value);
  bool Erase(const EbbId& key);
  bool Erase(ConstAccessor& item_accessor);
  bool Erase(Accessor& item_accessor);

 private:
  MapType map_;
};

constexpr auto local_id_map = EbbRef<LocalIdMap>(kLocalIdMapId);
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_LOCALIDMAP_H_
