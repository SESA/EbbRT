//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Runtime.h>

#include <capnp/message.h>

#include <ebbrt/BootFdt.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/Net.h>
#include <ebbrt/RuntimeInfo.capnp.h>

namespace {
class MutableBufferListMessageReader : public capnp::MessageReader {
 public:
  MutableBufferListMessageReader(
      const ebbrt::MutableBufferList& l,
      capnp::ReaderOptions options = capnp::ReaderOptions())
      : MessageReader(options), l_(l) {}

  virtual kj::ArrayPtr<const capnp::word> getSegment(uint id) override {
    auto it = l_.begin();
    for (uint i = 0; i < id; ++i) {
      if (it == l_.end())
        return nullptr;
      ++it;
    }
    ebbrt::kbugon(it->size() % sizeof(capnp::word) != 0,
                  "buffer must be word aligned\n");
    return kj::ArrayPtr<const capnp::word>(
        static_cast<const capnp::word*>(it->addr()),
        it->size() / sizeof(capnp::word));
  }

 private:
  const ebbrt::MutableBufferList& l_;
};
}  // namespace

void ebbrt::runtime::Init() {
  auto reader = boot_fdt::Get();
  auto offset = reader.GetNodeOffset("/runtime");
  auto ip = reader.GetProperty32(offset, "address");
  auto port = reader.GetProperty16(offset, "port");

  auto pcb = new NetworkManager::TcpPcb();
  ip_addr_t addr;
  addr.addr = htonl(ip);
  EventManager::EventContext context;
  pcb->Receive([pcb, &context](MutableBufferList l) {
    if (l.empty()) {
      delete pcb;
    } else {
      auto message = MutableBufferListMessageReader(l);
      auto info = message.getRoot<RuntimeInfo>();

      ebb_allocator->SetIdSpace(info.getEbbIdSpace());
      kprintf("%x:%d\n", info.getGlobalMapAddress(), info.getGlobalMapPort());
      global_id_map->Connect(info.getGlobalMapAddress(),
                             info.getGlobalMapPort());
      // global_id_map->Connect(htons(info.getGlobalMapPort()),
      //                        htonl(info.getGlobalMapAddress()));
      event_manager->ActivateContext(context);
    }
  });
  pcb->Connect(&addr, port);
  event_manager->SaveContext(context);
}
