//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Printer.h"

#include <ebbrt/LocalIdMap.h>
#include <ebbrt/GlobalIdMap.h>

EBBRT_PUBLISH_TYPE(, Printer);

Printer::Printer(ebbrt::Messenger::NetworkId nid)
    : Messagable<Printer>(kPrinterEbbId), remote_nid_(std::move(nid)) {}

Printer& Printer::HandleFault(ebbrt::EbbId id) {
  {
    ebbrt::LocalIdMap::ConstAccessor accessor;
    auto found = ebbrt::local_id_map->Find(accessor, id);
    if (found) {
      auto& pr = *boost::any_cast<Printer*>(accessor->second);
      ebbrt::EbbRef<Printer>::CacheRef(id, pr);
      return pr;
    }
  }

  ebbrt::kprintf("Getting from gmap\n");
  ebbrt::EventManager::EventContext context;
  auto f = ebbrt::global_id_map->Get(id);
  Printer* p;
  f.Then([&f, &context, &p](ebbrt::Future<std::string> inner) {
    ebbrt::kprintf("Got from gmap\n");
    p = new Printer(ebbrt::Messenger::NetworkId(inner.Get()));
    ebbrt::event_manager->ActivateContext(context);
  });
  ebbrt::event_manager->SaveContext(context);
  ebbrt::kprintf("Awoke with data from gmap\n");
  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<Printer>::CacheRef(id, *p);
    return *p;
  }

  delete p;
  // retry reading
  ebbrt::LocalIdMap::ConstAccessor accessor;
  ebbrt::local_id_map->Find(accessor, id);
  auto& pr = *boost::any_cast<Printer*>(accessor->second);
  ebbrt::EbbRef<Printer>::CacheRef(id, pr);
  return pr;
}

void Printer::Print(std::string str) {
  auto ptr = str.data();
  auto sz = str.size();
  auto buf = std::make_shared<ebbrt::Buffer>(
      static_cast<void*>(const_cast<char*>(ptr)), sz,
      ebbrt::MoveBind([](std::string str, void* addr, size_t sz) {},
                      std::move(str)));
  SendMessage(remote_nid_, std::move(buf));
}

void Printer::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                             ebbrt::Buffer buffer) {
  throw std::runtime_error("Printer: Received message unexpectedly!");
}
