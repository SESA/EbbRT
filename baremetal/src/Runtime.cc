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
#include <ebbrt/Multiboot.h>
#include <ebbrt/Net.h>
#include <ebbrt/UniqueIOBuf.h>

#include <cstdlib>
#include <string>

namespace ebbrt {
namespace runtime {
uint32_t frontend;
}
}

uint32_t ebbrt::runtime::Frontend() { return frontend; }

namespace {
class Connection : public ebbrt::TcpHandler {
 public:
  explicit Connection(ebbrt::NetworkManager::TcpPcb&& pcb,  // NOLINT
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
#if __EBBRT_ENABLE_FDT__
  auto reader = boot_fdt::Get();
  auto offset = reader.GetNodeOffset("/runtime");
  auto ip = reader.GetProperty32(offset, "address");
  auto port = reader.GetProperty16(offset, "port");
  auto allocation_id = reader.GetProperty16(offset, "allocation_id");
  auto addr = Ipv4Address(htonl(ip));
#else
  auto cmdline = std::string(ebbrt::multiboot::CmdLine());
  auto host = std::string("host_addr=");
  auto loc = cmdline.find(host);
  if (loc == std::string::npos) {
    kabort("No host address found in command line\n");
  }
  host = cmdline.substr((loc + host.size()));
  auto gap = host.find(";");
  if (gap != std::string::npos) {
    host = host.substr(0, gap);
  }

  auto port = std::string("host_port=");
  loc = cmdline.find(port);
  if (loc == std::string::npos) {
    kabort("No host port found in command line\n");
  }
  port = cmdline.substr((loc + port.size()));
  gap = port.find(";");
  if (gap != std::string::npos) {
    port = port.substr(0, gap);
  }

  auto idstr = std::string("allocid=");
  loc = cmdline.find(idstr);
  if (loc == std::string::npos) {
    kabort("No allocation id found in command line\n");
  }
  idstr = cmdline.substr((loc + idstr.size()));
  gap = idstr.find(";");
  if (gap != std::string::npos) {
    idstr = idstr.substr(0, gap);
  }

  auto allocation_id = atoi(idstr.c_str());
  auto nport = atoi(port.c_str());
  auto naddr = atoi(host.c_str());
  auto addr = Ipv4Address(htonl(naddr));
#endif
  kprintf("RUNTIME command line args: %u %u %u\n", naddr, nport, allocation_id);

  EventManager::EventContext context;
  NetworkManager::TcpPcb pcb;
  pcb.Connect(addr, nport);
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
