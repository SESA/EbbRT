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
    auto addr = pcb.GetRemoteAddress().addr;
    std::lock_guard<ebbrt::SpinLock> lock(lock_);
    if (connection_map_.find(addr) != connection_map_.end())
      throw std::runtime_error("Store to promise");

    pcb.Receive([this](NetworkManager::TcpPcb& t, std::unique_ptr<IOBuf>&& b) {
      Receive(t, std::move(b));
    });
    connection_map_.emplace(
        addr, MakeReadyFuture<NetworkManager::TcpPcb>(std::move(pcb)).Share());
  });
}

ebbrt::Messenger::NetworkId ebbrt::Messenger::LocalNetworkId() {
  ebbrt::NetworkManager::Interface iface =
      ebbrt::network_manager->FirstInterface();
  return NetworkId(iface.IPV4Addr());
}

void ebbrt::Messenger::Receive(NetworkManager::TcpPcb& t,
                               std::unique_ptr<IOBuf>&& b) {
  // This may be part of a message we still haven't received all of, check if we
  // queued anything
  auto addr = t.GetRemoteAddress().addr;
  auto it = queued_receives_.find(addr);
  bool in_queue = false;
  std::unique_ptr<IOBuf>* buf;
  if (it != queued_receives_.end()) {
    it->second->Prev()->AppendChain(std::move(b));
    in_queue = true;
    buf = &it->second;
  } else {
    buf = &b;
  }

  kbugon((*buf)->Length() < sizeof(Header), "buffer too small\n");
  auto p = (*buf)->GetDataPointer();
  auto& header = p.Get<Header>();
  auto message_len = sizeof(Header) + header.length;
  auto chain_len = (*buf)->ComputeChainDataLength();
  kbugon(chain_len > message_len, "Need to leave tail of message around\n");
  if (chain_len < message_len && !in_queue) {
    // we need to put the data in the queue
    queued_receives_.emplace(addr, std::move(*buf));
  } else if (chain_len == message_len) {
    // we are good to upcall the receiver with the message
    auto& ref = GetMessagableRef(header.id, header.type_code);
    // consume header
    (*buf)->Advance(sizeof(Header));
    ref.ReceiveMessageInternal(NetworkId(ntohl(t.GetRemoteAddress().addr)),
                               std::move(*buf));
    if (in_queue) {
      queued_receives_.erase(it);
    }
  }
}

ebbrt::Future<void> ebbrt::Messenger::Send(NetworkId to, EbbId id,
                                           uint64_t type_code,
                                           std::unique_ptr<IOBuf>&& data) {
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
  buf->AppendChain(std::move(data));

  return connection_map_[to.ip.addr].Then(
      MoveBind([](std::unique_ptr<IOBuf>&& data,
                  SharedFuture<NetworkManager::TcpPcb> f) {
                 f.Get().Send(std::move(data));
               },
               std::move(buf)));
}
