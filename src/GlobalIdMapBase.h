#ifndef COMMON_SRC_INCLUDE_EBBRT_GLOBALIDMAPBASE_H_
#define COMMON_SRC_INCLUDE_EBBRT_GLOBALIDMAPBASE_H_

#include "Future.h"
#include "LocalIdMap.h"

// Virtual base class for polymorphic GlobalIdMap Ebb
namespace ebbrt {

extern void InstallGlobalIdMap();

class GlobalIdMap {
  // Base class used in Ebb translation
 public:
  class Base {
   public:
    virtual GlobalIdMap& Construct(EbbId id) = 0;

   private:
    friend class GlobalIdMap;
  };
  // Miss uses base class from the local_id_map to construct the rep
  // This base class instance was previously constructed by the child type
  static GlobalIdMap& HandleFault(EbbId id) {
    LocalIdMap::ConstAccessor accessor;
    auto found = local_id_map->Find(accessor, id);
    if (!found)
      throw std::runtime_error("Failed to find base type for GlobalIdBase");
    auto base = boost::any_cast<Base*>(accessor->second);
    return base->Construct(id);
  }
  virtual Future<std::string> Get(EbbId id, std::string path) = 0;
  virtual Future<void> Set(EbbId id, std::string data, std::string path) = 0;
};
}
#endif  // COMMON_SRC_INCLUDE_EBBRT_GLOBALIDMAPBASE_H_
