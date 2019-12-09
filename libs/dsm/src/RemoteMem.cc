#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/IOBuf.h>
#include "RemoteMem.h"

RemoteMemory &
RemoteMemory::HandleFault(ebbrt::EbbId id){
  {
    // Trying to find if registered in the local Id map
    ebbrt::LocalIdMap::ConstAccessor accessor;
    auto found = ebbrt::local_id_map->Find(accessor, id);
    if (found) {
      // If found the cache reference and return the rep pointer
      auto& r = *boost::any_cast<RemoteMemory*>(accessor->second);
      ebbrt::EbbRef<RemoteMemory>::CacheRef(id, r);
      return r;
    }
  }
  // join with home node, found in global_id_map
#ifdef __ebbrt__
  ebbrt::EventManager::EventContext context;
  auto f = ebbrt::global_id_map->Get(id);
  RemoteMemory* p;
  f.Then([&f, &context, &p, id](ebbrt::Future<std::string> inner) {
    p = new RemoteMemory();
    p->addTo(ebbrt::Messenger::NetworkId(inner.Get()));
    ebbrt::event_manager->ActivateContext(std::move(context));
  });
  ebbrt::event_manager->SaveContext(context);
  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<RemoteMemory>::CacheRef(id, *p);
    return *p;
  }
#else
  RemoteMemory* p;
  ebbrt::global_id_map->Set(id, ebbrt::messenger->LocalNetworkId().ToBytes());
  p = new RemoteMemory();
  ebbrt::EbbRef<RemoteMemory>::CacheRef(id, *p);
  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<RemoteMemory>::CacheRef(id, *p);
    return *p;
  }
#endif
  delete p;
  // retry reading
  ebbrt::LocalIdMap::ConstAccessor accessor;
  ebbrt::local_id_map->Find(accessor, id);
  auto& r = *boost::any_cast<RemoteMemory*>(accessor->second);
  ebbrt::EbbRef<RemoteMemory>::CacheRef(id, r);
  return r;
}


ebbrt::Future<int> RemoteMemory::QueryMaster(int i){
  //this join is a consistent join that hold untill the node itself knows its joined
  //ebbrt::kprintf("joining the home node\n");
  int id;
  ebbrt::Promise<int> promise;
  auto f = promise.GetFuture();
  {
    std::lock_guard<std::mutex> guard(lock_);
    if (i == 0){
#ifdef __ebbrt__
      int p = 32765;
      ebbrt::kprintf("Start Listening on port %d\n", p);
      ebbrt::messenger->StartListening(p);
#endif
      id = 10;
    }else{
      id = 8;
    }
    bool inserted;
    std::tie(std::ignore, inserted) =
      promise_map_.emplace(id, std::move(promise));
    assert(inserted);
  }
  auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t));
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint32_t>() = i; 
  SendMessage(nodelist[0], std::move(buf));
  return f;
}

void RemoteMemory::sendPage(ebbrt::Messenger::NetworkId dst){

  auto buffer = ebbrt::MakeUniqueIOBuf(sizeof(uint32_t)+sizeof(uint64_t)+sizeof(uint64_t)+tem_buffer[0]);
  ebbrt::kprintf("size of buffer = %d, len = %d, loop = %d\n", sizeof(buffer), tem_buffer[0], tem_buffer[1]);
  auto dp = buffer->GetMutDataPointer();
  dp.Get<uint32_t>() = 8;
  dp.Get<uint64_t>() = tem_buffer[0];
  dp.Get<uint64_t>() = tem_buffer[1];
  for (uint64_t i = 0; i < tem_buffer[1]; i++){
    dp.Get<uint32_t>() = tem_buffer[i+2];
    // ebbrt::kprintf("BM: value read into buffer 0x%llx\n", tem_buffer[i+2]);
  }
  ebbrt::kprintf("destination nid: %s\n", dst.ToString().c_str());  

  SendMessage(dst, std::move(buffer));
}

void RemoteMemory::cachePage(uint64_t len, volatile uint32_t * pptr, uint64_t iteration){
  ebbrt::kprintf("Cached the paged\n");

  tem_buffer.push_back(len);
  tem_buffer.push_back(iteration);
  for (uint64_t i = 0; i < iteration; i++){
    tem_buffer.push_back(*pptr);
    //ebbrt::kprintf("BM: value read into buffer 0x%llx\n", *pptr);
    pptr++;
  }
  ebbrt::kprintf("size of tem_buffer = %d, len = %d, loop = %d\n", tem_buffer.size(), len, iteration);
  cached = true;
  return;
}

void RemoteMemory::getPage(volatile uint32_t* pptr){
  if(tem_buffer.size() > 0){
    ebbrt::kprintf("iteration is %d\n", tem_buffer[1]);
    for (uint64_t i = 0; i < tem_buffer[1]; i++){
      *pptr = tem_buffer[i+2];
      //ebbrt::kprintf("setting physical page value 0x%x\n", *pptr);
      pptr++;
    }
  }else{
    ebbrt::kabort("did not get a page");
  }
}

void RemoteMemory::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
			 std::unique_ptr<ebbrt::IOBuf>&& buffer){
  auto dp = buffer->GetDataPointer();
  auto id = dp.Get<uint32_t>();
  ebbrt::kprintf("id is %d\n", id);

  if(id == 0){
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
    auto dp = buf->GetMutDataPointer();
    addTo(nid);
    if (size() == 1) {
      dp.Get<uint32_t>() = 10;
      }else{
      dp.Get<uint32_t>() = 11;
    }
    dp.Get<uint32_t>() = 10;
    SendMessage(nid, std::move(buf));
    return;
  }

  if (id == 8){
#ifdef __ebbrt__
    auto len = dp.Get<uint64_t>();
    auto iteration = dp.Get<uint64_t>();
    tem_buffer.push_back(len);
    tem_buffer.push_back(iteration);
    for(uint64_t i = 0; i < iteration; i++){
      tem_buffer.push_back(dp.Get<uint32_t>());
    }
    auto it = promise_map_.find(8);
    assert(it != promise_map_.end());
    it->second.SetValue(1);
    promise_map_.erase(it);

#else
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(uint64_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = 9;
    char dst[20];
    std::strcpy(dst, nid.ToString().c_str());
    auto ip = std::strtok(dst, ".");
    while(ip != NULL){
      dp.Get<uint8_t>() = std::atoi(ip);
      ip = std::strtok(NULL, ".");
    }
    SendMessage(nodelist[0], std::move(buf));
#endif
    return;
  }

  if (id == 9){
#ifdef __ebbrt__
    if (cached){
	std::array<uint8_t, 4> ar_ip = {0, 0, 0, 0};
	for(int i = 0; i < 4; i++){
	  ar_ip[i] = dp.Get<uint8_t>();
	}
	auto dst_nid = ebbrt::Messenger::NetworkId(std::string(reinterpret_cast<const char*>(ar_ip.data()), 4));
	sendPage(dst_nid);
    }
#endif
    return;
  }

  if(id >= 10 && id < 12){
    auto trans_num = dp.Get<uint32_t>();
    auto it = promise_map_.find(trans_num);
    assert(it != promise_map_.end());
    it->second.SetValue(id);
    promise_map_.erase(it);
    return;
  }
}
