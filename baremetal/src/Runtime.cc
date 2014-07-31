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
#include "RuntimeInfo.capnp.h"

char* ebbrt::runtime::bootcmdline;

namespace {
uint32_t frontend;
}

uint32_t ebbrt::runtime::Frontend() { return frontend; }

void ebbrt::runtime::Init() {
  auto runtime_override = []() {
    auto reader = ebbrt::boot_fdt::Get();
    auto offset = reader.GetNodeOffset("/runtime");
    if (!offset)
      return false;
    auto sid = reader.GetProperty16(*offset, "EbbIDSpace");
    if (!sid)
      return false;
    ebbrt::ebb_allocator->SetIdSpace(*sid);
    auto GlobalIdMapIP = reader.GetProperty32(*offset, "GlobalIdMapIP");
    if (!GlobalIdMapIP)
      return false;
    frontend = *GlobalIdMapIP;
    auto GlobalIdMapPort = reader.GetProperty16(*offset, "GlobalIdMapPort");
    if (!GlobalIdMapPort)
      return false;
    auto port = *GlobalIdMapPort;
    ebbrt::kprintf("%x:%d\n", frontend, port);
    ebbrt::messenger->StartListening(port);
    ebbrt::global_id_map->SetAddress(frontend);
    return true;
  };

  if (!runtime_override()) {
    auto reader = boot_fdt::Get();
    auto offset = *reader.GetNodeOffset("/runtime");
    auto ip = *reader.GetProperty32(offset, "address");
    auto port = *reader.GetProperty16(offset, "port");
    auto allocation_id = *reader.GetProperty16(offset, "allocation_id");

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
    auto buf = IOBuf::Create(sizeof(uint16_t));
    *reinterpret_cast<uint16_t*>(buf->WritableData()) = htons(allocation_id);
    pcb->Send(std::move(buf));
  }
}
