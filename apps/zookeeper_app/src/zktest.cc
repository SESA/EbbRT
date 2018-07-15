//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <signal.h>
#include <chrono>

#include <ebbrt/Cpu.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EventManager.h>

#include <ebbrt-zookeeper/ZKGlobalIdMap.h>

void zkChildTest(){
 ebbrt::EbbId testid = ebbrt::ebb_allocator->AllocateLocal();

 ebbrt::ZKGlobalIdMap::ZKOptArgs args;
 args.data = "Child test";
 ebbrt::ZKGlobalIdMap::ZKOptArgs c1_args;
 ebbrt::ZKGlobalIdMap::ZKOptArgs c2_args;
 ebbrt::ZKGlobalIdMap::ZKOptArgs c3_args;
 c1_args.data = "A";
 c1_args.path = "C1";
 c2_args.data = "B";
 c2_args.path = "C2";
 c3_args.data = "C";
 c3_args.path = "C3";
 
 ebbrt::zkglobal_id_map->Set(testid, args);
 ebbrt::zkglobal_id_map->Set(testid, c1_args);
 ebbrt::zkglobal_id_map->Set(testid, c2_args);
 ebbrt::zkglobal_id_map->Set(testid, c3_args);

 ebbrt::ZKGlobalIdMap::ZKOptArgs temp;
 auto child_vec = ebbrt::zkglobal_id_map->List(testid, temp).Block().Get();
 for ( auto child : child_vec ){
  temp.path = child; 
  auto val = ebbrt::zkglobal_id_map->Get(testid, temp).Block().Get();
  std::cout << child << ": " << val << std::endl;
 }
 std::cout << "Finish zkChildTest!\n" << std::endl;
}

void zkThreadTest(){


 auto cpu_num = ebbrt::Cpu::GetPhysCpus();
 std::cout << "Running zkThreadTest #" << cpu_num << std::endl;
 ebbrt::EbbId testid = ebbrt::ebb_allocator->AllocateLocal();
 for (int i = 0; i < cpu_num; i++) {
  auto cpu_context = ebbrt::Cpu::GetByIndex(i)->get_context();
  ebbrt::event_manager->Spawn([testid ]() { 
/* BEGIN CORE SPECIFIC CODE */	
   size_t myCore = ebbrt::Cpu::GetMine();
   std::cout << "Start:" << myCore << std::endl;
   if (myCore == 2) {
	ebbrt::ZKGlobalIdMap::ZKOptArgs args;
	args.data = "Head";
	std::cout << "Set: " << myCore << std::endl;
	ebbrt::zkglobal_id_map->Set(testid, args);
   } else {
	ebbrt::ZKGlobalIdMap::ZKOptArgs temp;
	auto isPresent = ebbrt::zkglobal_id_map->Exists(testid, temp).Block().Get();
	if ( isPresent ){
	std::cout << "YES:" << myCore<< std::endl;
	}else{
	std::cout << "NO:" << myCore<< std::endl;
	}
   }
/* END CORE SPECIFIC CODE */
  }, cpu_context, true);
 }
}

void AppMain() {

  auto start = std::chrono::steady_clock::now();

  zkChildTest();
  zkThreadTest();

  auto end = std::chrono::duration_cast<std::chrono::milliseconds> 
                            (std::chrono::steady_clock::now() - start);
  std::cout << "TOTAL TIME: " << end.count() << std::endl;

}

int main(int argc, char **argv) {
  void *status;
  pthread_t tid = ebbrt::Cpu::EarlyInit(5);
  pthread_join(tid, &status);
  return 0;
}
