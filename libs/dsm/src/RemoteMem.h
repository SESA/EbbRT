#include <ebbrt/Future.h>
#include "StaticEbbIds.h"
#include <ebbrt/Message.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/Debug.h>
#include "StaticEbbIds.h"
#include <unordered_map>


class RemoteMemory : public ebbrt::Messagable<RemoteMemory>{
 private:
  std::mutex lock_;
  std::vector<ebbrt::Messenger::NetworkId> nodelist;
  bool cached = false;
  std::vector<uint64_t> tem_buffer;
  std::unordered_map<uint32_t, ebbrt::Promise<int>> promise_map_;
  RemoteMemory() : ebbrt::Messagable<RemoteMemory>(kRemoteMEbbId), nodelist{} {}
  void addTo(ebbrt::Messenger::NetworkId nid){
    if (std::find(nodelist.begin(), nodelist.end(), nid) == nodelist.end()){
      nodelist.push_back(nid);
    }
  }

 public:
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid, std::unique_ptr<ebbrt::IOBuf>&& buffer);
  static RemoteMemory & HandleFault(ebbrt::EbbId id);
  ebbrt::Future<int> QueryMaster(int i);
  int size(){ return int(nodelist.size()); }  
  void sendPage(ebbrt::Messenger::NetworkId dst);
  void getPage(volatile uint32_t* pptr);
  void cachePage(uint64_t len, volatile uint32_t * pptr, uint64_t iteration);
};
constexpr auto rm = ebbrt::EbbRef<RemoteMemory>(kRemoteMEbbId);
