//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Messenger.h>

#include <ebbrt/Debug.h>
#include <ebbrt/GlobalIdMap.h>

uint16_t ebbrt::Messenger::port_;

ebbrt::Messenger::Messenger() {}

void ebbrt::Messenger::StartListening(uint16_t port) {
  port_ = port;
  tcp_.Bind(port);
  tcp_.Listen();
  tcp_.Accept([this](NetworkManager::TcpPcb pcb) {
    kprintf("New Messenger Connection\n");
  });
}

void ebbrt::Messenger::Receive(NetworkManager::TcpPcb& t, Buffer b) {
  kbugon(b.length() > 1, "handle multiple length buffer\n");
  kbugon(b.size() < sizeof(Header), "buffer too small\n");
  auto p = b.GetDataPointer();
  auto& header = p.Get<Header>();
  kprintf("Received %d\n", header.length);
  auto& ref = GetMessagableRef(header.id, header.type_code);
  kbugon(b.size() < (sizeof(Header) + header.length),
         "Did not receive complete message\n");
  // consume header
  b += sizeof(Header);
  ref.ReceiveMessageInternal(NetworkId(ntohl(t.GetRemoteAddress().addr)),
                             std::move(b));
}

ebbrt::Future<void> ebbrt::Messenger::Send(NetworkId to, EbbId id,
                                           uint64_t type_code,
                                           std::shared_ptr<const Buffer> data) {
  // make sure we have a pending connection
  {
    std::lock_guard<SpinLock> lock(lock_);
    auto it = connection_map_.find(to.ip.addr);
    if (it == connection_map_.end()) {
      // we don't have a pending connection, start one
      NetworkManager::TcpPcb pcb;
      pcb.Receive([this](NetworkManager::TcpPcb& t,
                         Buffer b) { Receive(t, std::move(b)); });
      auto f = pcb.Connect(&to.ip, port_);
      auto f2 = f.Then(MoveBind([](NetworkManager::TcpPcb pcb, Future<void> f) {
                                  kprintf("Connected\n");
                                  f.Get();
                                  return pcb;
                                },
                                std::move(pcb)));
      connection_map_.emplace(to.ip.addr, std::move(f2.Share()));
    }
  }

  // construct header
  auto buf = std::make_shared<Buffer>(sizeof(Header));
  auto h = static_cast<Header*>(buf->data());
  h->length = data->total_size();
  h->type_code = type_code;
  h->id = id;
  buf->append(std::move(std::const_pointer_cast<Buffer>(data)));

  return connection_map_[to.ip.addr].Then(
      MoveBind([](std::shared_ptr<const Buffer> data,
                  SharedFuture<NetworkManager::TcpPcb> f) {
                 kprintf("Sending %d\n", data->total_size());
                 f.Get().Send(std::move(data));
               },
               std::move(buf)));
}
