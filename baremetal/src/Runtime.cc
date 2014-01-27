//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Runtime.h>

#include <capnp/message.h>

#include <ebbrt/Debug.h>
#include <ebbrt/Net.h>
#include <ebbrt/RuntimeInfo.capnp.h>

namespace {
class MutableBufferListMessageReader : public capnp::MessageReader {
 public:
  MutableBufferListMessageReader(
      const ebbrt::MutableBufferList &l,
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
        static_cast<const capnp::word *>(it->addr()),
        it->size() / sizeof(capnp::word));
  }

 private:
  const ebbrt::MutableBufferList &l_;
};
}  // namespace

void ebbrt::runtime::Init() {
  auto pcb = new NetworkManager::TcpPcb();
  struct ip_addr addr;
  IP4_ADDR(&addr, 10, 128, 128, 1);
  pcb->Receive([pcb](MutableBufferList l) {
    if (l.empty()) {
      delete pcb;
    } else {
      // auto message = MutableBufferListMessageReader(l);
      // auto info = message.getRoot<RuntimeInfo>();

      // ebb_allocator->setIdSpace(info.getEbbIdSpace());
      // global_map->setaddress(htons(info.getGlobalMapPort()),
      //                        htonl(info.getGlobalMapAddress()));
    }
  });
  pcb->Connect(&addr, 46454);
}
