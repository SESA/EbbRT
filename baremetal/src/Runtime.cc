//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Runtime.h>

#include <ebbrt/BootFdt.h>
#include <ebbrt/CapnpMessage.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/Net.h>
#include <ebbrt/RuntimeInfo.capnp.h>

namespace ebbrt {
  namespace runtime {
    uint32_t frontend;
  }
}

uint32_t ebbrt::runtime::Frontend() { return frontend; }

void ebbrt::runtime::Init() {

  auto reader = boot_fdt::Get();
  auto offset = reader.GetNodeOffset("/runtime");
  auto ip = reader.GetProperty32(offset, "address");
  auto port = reader.GetProperty16(offset, "port");

  auto pcb = new NetworkManager::TcpPcb();
  ip_addr_t addr;
  addr.addr = htonl(ip);
  EventManager::EventContext context;
  pcb->Receive([pcb, &context](NetworkManager::TcpPcb& t,
                               std::unique_ptr<IOBuf>&& b) {
    if (b->Length() == 0) {
      delete pcb;
    } else {
      auto message = IOBufMessageReader(std::move(b));
      auto info = message.getRoot<RuntimeInfo>();

      ebb_allocator->SetIdSpace(info.getEbbIdSpace());
      frontend = info.getGlobalIdMapAddress();
      const auto& port = info.getMessengerPort();
      kprintf("%x:%d\n", frontend, port);
      messenger->StartListening(port);
      global_id_map->SetAddress(frontend);
      event_manager->ActivateContext(std::move(context));
    }
  });
  pcb->Connect(&addr, port);
  event_manager->SaveContext(context);
}
