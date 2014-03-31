//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_
#define HOSTED_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_

#include <string>
#include <utility>

#include <ebbrt/Buffer.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/Future.h>
#include <ebbrt/Message.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {
class GlobalIdMap : public StaticSharedEbb<GlobalIdMap>,
                    public CacheAligned,
                    public Messagable<GlobalIdMap> {
 public:
  GlobalIdMap();

  Future<std::string> Get(EbbId id);
  Future<void> Set(EbbId id, std::string data);
  Future<std::string> Get(EbbId id);

  void ReceiveMessage(Messenger::NetworkId nid, std::unique_ptr<IOBuf>&& buf);

 private:
  std::mutex m_;
  std::unordered_map<EbbId, std::string> map_;
};

const constexpr auto global_id_map = EbbRef<GlobalIdMap>(kGlobalIdMapId);
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_GLOBALIDMAP_H_
