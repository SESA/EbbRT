//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Messenger.h>

#include <ebbrt/Debug.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/Net.h>

uint16_t ebbrt::Messenger::port_;

ebbrt::Messenger::Messenger() {}

void ebbrt::Messenger::StartListening(uint16_t port) {
  port_ = port;
  tcp_.Bind(port);
  tcp_.Listen();
  tcp_.Accept([this](NetworkManager::TcpPcb pcb) {
    kabort("UNIMPLEMENTED: New Messenger Connection\n");
  });
}

ebbrt::Messenger::NetworkId 
ebbrt::Messenger::LocalNetworkId() {
  ebbrt::NetworkManager::Interface iface = 
    ebbrt::network_manager->FirstInterface();  
  return NetworkId(iface.IPV4Addr());
}

void ebbrt::Messenger::Receive(NetworkManager::TcpPcb& t,
                               std::unique_ptr<IOBuf>&& b) {
  kbugon(b->CountChainElements() > 1, "handle multiple length buffer\n");
  kbugon(b->Length() < sizeof(Header), "buffer too small\n");
  auto p = b->GetDataPointer();
  auto& header = p.Get<Header>();
  auto& ref = GetMessagableRef(header.id, header.type_code);
  kbugon(b->Length() < (sizeof(Header) + header.length),
         "Did not receive complete message\n");
  // consume header
  b->Advance(sizeof(Header));
  ref.ReceiveMessageInternal(NetworkId(ntohl(t.GetRemoteAddress().addr)),
                             std::move(b));
}

ebbrt::Future<void>
ebbrt::Messenger::Send(NetworkId to, EbbId id, uint64_t type_code,
                       std::unique_ptr<const IOBuf>&& data) {
  // make sure we have a pending connection
  {
    std::lock_guard<SpinLock> lock(lock_);
    auto it = connection_map_.find(to.ip.addr);
    if (it == connection_map_.end()) {
      // we don't have a pending connection, start one
      NetworkManager::TcpPcb pcb;
      pcb.Receive([this](NetworkManager::TcpPcb& t,
                         std::unique_ptr<IOBuf>&& b) {
        Receive(t, std::move(b));
      });
      auto f = pcb.Connect(&to.ip, port_);
      auto f2 = f.Then(MoveBind([](NetworkManager::TcpPcb pcb, Future<void> f) {
                                  f.Get();
                                  return pcb;
                                },
                                std::move(pcb)));
      connection_map_.emplace(to.ip.addr, std::move(f2.Share()));
    }
  }

  // construct header
  auto buf = IOBuf::Create(sizeof(Header));
  auto h = reinterpret_cast<Header*>(buf->WritableData());
  h->length = data->ComputeChainDataLength();
  h->type_code = type_code;
  h->id = id;
  // Cast to non const is ok because we then take the whole chain as const after
  buf->AppendChain(std::unique_ptr<IOBuf>(const_cast<IOBuf*>(data.release())));

  return connection_map_[to.ip.addr].Then(
      MoveBind([](std::unique_ptr<const IOBuf>&& data,
                  SharedFuture<NetworkManager::TcpPcb> f) {
                 f.Get().Send(std::move(data));
               },
               std::move(buf)));
}
