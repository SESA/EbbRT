//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Runtime.h>

#include "RuntimeInfo.capnp.h"
#include <ebbrt/BootFdt.h>
#include <ebbrt/CapnpMessage.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/Net.h>
#include <ebbrt/UniqueIOBuf.h>

namespace ebbrt {
namespace runtime {
uint32_t frontend;
}
}

uint32_t ebbrt::runtime::Frontend() { return frontend; }

namespace {
class Connection : public ebbrt::TcpHandler {
 public:
  explicit Connection(ebbrt::NetworkManager::TcpPcb&& pcb,
                      ebbrt::EventManager::EventContext& context)
      : TcpHandler(std::move(pcb)), context_(context) {}

  void Receive(std::unique_ptr<ebbrt::MutIOBuf> b) override {
    auto message = ebbrt::IOBufMessageReader(std::move(b));
    auto info = message.getRoot<RuntimeInfo>();

    ebbrt::ebb_allocator->SetIdSpace(info.getEbbIdSpace());
    ebbrt::runtime::frontend = info.getGlobalIdMapAddress();
    const auto& port = info.getMessengerPort();
    ebbrt::kprintf("%x:%d\n", ebbrt::runtime::frontend, port);
    ebbrt::messenger->StartListening(port);
    ebbrt::global_id_map->SetAddress(ebbrt::runtime::frontend);
    ebbrt::event_manager->ActivateContext(std::move(context_));
  }

  void Close() override { Shutdown(); }

  void Abort() override {
    ebbrt::kabort("UNIMPLEMENTED: Runtime connection aborted\n");
  }

 private:
  ebbrt::EventManager::EventContext& context_;
};
}  // namespace

void ebbrt::runtime::Init() {
  auto reader = boot_fdt::Get();
  auto offset = reader.GetNodeOffset("/runtime");
  auto ip = reader.GetProperty32(offset, "address");
  auto port = reader.GetProperty16(offset, "port");
  auto allocation_id = reader.GetProperty16(offset, "allocation_id");

  auto addr = Ipv4Address(htonl(ip));
  EventManager::EventContext context;
  NetworkManager::TcpPcb pcb;
  pcb.Connect(addr, port);
  auto connection = new Connection(std::move(pcb), context);
  connection->Install();
  event_manager->SaveContext(context);

  // Send reply back
  auto buf = MakeUniqueIOBuf(sizeof(uint16_t));
  auto dp = buf->GetMutDataPointer();
  dp.Get<uint16_t>() = htons(allocation_id);
  connection->Send(std::move(buf));
  connection->Pcb().Output();
}
