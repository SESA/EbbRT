#include <ebbrt-zookeeper/ZKGlobalIdMap.h>
#include <ebbrt/EventManager.h>

void ebbrt::InstallGlobalIdMap(){
  ebbrt::kprintf("Installing ZooKeeper GlobalIdMap\n");
  ebbrt::ZKGlobalIdMap::Create(ebbrt::kGlobalIdMapId);
	// Do this asynchronously to allow clean bring-up of CPU context 
  // TODO: do not spawn
#ifndef __ebbrt__
  ebbrt::event_manager->Spawn([]() { zkglobal_id_map->Init();/*.Block();*/ });
#else
  zkglobal_id_map->Init();
#endif
}
