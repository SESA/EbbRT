//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Messenger.h>

#include <ebbrt/Debug.h>
#include <ebbrt/Message.h>
#include <ebbrt/SharedIOBufRef.h>
#include <ebbrt/UniqueIOBuf.h>

uint16_t ebbrt::Messenger::port_;

ebbrt::Messenger::Messenger() {}

ebbrt::Messenger::Connection::Connection(ebbrt::NetworkManager::TcpPcb pcb)
    : TcpHandler(std::move(pcb)) {}

void ebbrt::Messenger::Connection::preallocated(std::unique_ptr<MutIOBuf> b) {

  auto len = b->Length();
  buf_->Advance(preallocate_);
  std::memcpy(reinterpret_cast<void*>(buf_->MutData()), b->Data(), len);
  buf_->Retreat(preallocate_);

  auto dp = buf_->GetMutDataPointer();
  preallocate_ += len;
  auto& header = dp.Get<Header>();
  auto message_len = sizeof(Header) + header.length;

  if (preallocate_ == message_len) {
    kassert(buf_->Length() == message_len);
    // pass along received message
    std::unique_ptr<MutIOBuf> msg;
    msg = std::move(buf_);
    preallocate_ = 0;
    msg->AdvanceChain(sizeof(Header));
    auto& ref = GetMessagableRef(header.id, header.type_code);
    ref.ReceiveMessageInternal(NetworkId(Pcb().GetRemoteAddress()),
                               std::move(msg));
  }
  kassert(preallocate_ <= message_len);
  return;
}

void ebbrt::Messenger::Connection::many_payloads(std::unique_ptr<MutIOBuf> b) {
  // FIXME: Support for multiple messages in a payload
  EBBRT_UNIMPLEMENTED();
  return;
}

void ebbrt::Messenger::Connection::Receive(std::unique_ptr<MutIOBuf> b) {
  kassert(b->Length() != 0);
  kassert(b->IsChained() == false);

  // processes preallocated message buffer
  if (preallocate_) {
    preallocated(std::move(b));
    return;
  }
  // process buffer chain
  if (buf_) {
    buf_->PrependChain(std::move(b));
  } else {
    buf_ = std::move(b);
  }
  // process message
  auto buffer_len = buf_->ComputeChainDataLength();
  if (buffer_len < sizeof(Header)) {
    return;
  }
  auto dp = buf_->GetDataPointer();
  auto& header = dp.Get<Header>();
  auto message_len = sizeof(Header) + header.length;

  // pass message along, or buffer partial message
  if (likely(buffer_len == message_len)) {
    std::unique_ptr<MutIOBuf> msg;
    msg = std::move(buf_);
    msg->AdvanceChain(sizeof(Header));
    auto& ref = GetMessagableRef(header.id, header.type_code);
    ref.ReceiveMessageInternal(NetworkId(Pcb().GetRemoteAddress()),
                               std::move(msg));
  } else if (buffer_len > message_len) {
    many_payloads(std::move(b));
  } else {
    // preallocate buffer if payload occupancy ratio drops below threshold
    if (buf_->CountChainElements() % kPreallocateChainLen == 0) {
      size_t capacity = 0;
      for (auto& buf : *buf_) {
        capacity += buf.Capacity();
      }
      auto ratio = (double)buffer_len / (double)capacity;
      if (ratio < kOccupancyRatio) {
        // allocate message buffer and coalesce chain
        auto newbuf = MakeUniqueIOBuf(message_len, false);
        auto dp = newbuf->GetMutDataPointer();
        for (auto& buf : *buf_) {
          auto len = buf.Length();
          std::memcpy(reinterpret_cast<void*>(dp.Data()), buf.Data(), len);
          dp.Advance(len);
          preallocate_ += len;
        }
        assert(newbuf->CountChainElements() == 1);
        assert(newbuf->ComputeChainDataLength() == message_len);
        assert(preallocate_ == buffer_len);
        buf_ = std::move(newbuf);
      }
    }
  }
  return;
}

void ebbrt::Messenger::Connection::Connected() { promise_.SetValue(this); }
// These need to remove themselves from the hash table
void ebbrt::Messenger::Connection::Close() {
  ebbrt::kabort("UNIMPLEMENTED: Messenger Close\n");
}
void ebbrt::Messenger::Connection::Abort() {
  ebbrt::kabort("UNIMPLEMENTED: Messenger Abort\n");
}

ebbrt::Future<ebbrt::Messenger::Connection*>
ebbrt::Messenger::Connection::GetFuture() {
  return promise_.GetFuture();
}

void ebbrt::Messenger::StartListening(uint16_t port) {
  port_ = port;
  listening_pcb_.Bind(port, [this](NetworkManager::TcpPcb pcb) {
    auto addr = pcb.GetRemoteAddress();
    std::lock_guard<ebbrt::SpinLock> lock(lock_);
    if (connection_map_.find(addr) != connection_map_.end())
      throw std::runtime_error("Store to promise");

    auto connection = new Connection(std::move(pcb));
    connection->Install();
    connection_map_.emplace(addr,
                            MakeReadyFuture<Connection*>(connection).Share());
  });
}

ebbrt::Messenger::NetworkId ebbrt::Messenger::LocalNetworkId() {
  // ebbrt::NetworkManager::Interface iface =
  //     ebbrt::network_manager->FirstInterface();
  // return NetworkId(iface.IPV4Addr());
  kabort("UNIMPLEMENTED\n");
}

ebbrt::Future<void> ebbrt::Messenger::Send(NetworkId to, EbbId id,
                                           uint64_t type_code,
                                           std::unique_ptr<IOBuf>&& data) {
  // make sure we have a pending connection
  {
    std::lock_guard<SpinLock> lock(lock_);
    auto it = connection_map_.find(to.ip);
    if (it == connection_map_.end()) {
      // we don't have a pending connection, start one
      NetworkManager::TcpPcb pcb;
      pcb.Connect(to.ip, port_);
      auto connection = new Connection(std::move(pcb));
      connection->Install();
      auto f = connection->GetFuture();
      connection_map_.emplace(to.ip, connection->GetFuture().Share());
    }
  }

  // construct header
  auto buf = MakeUniqueIOBuf(sizeof(Header));
  auto dp = buf->GetMutDataPointer();
  auto& h = dp.Get<Header>();
  h.length = data->ComputeChainDataLength();
  h.type_code = type_code;
  h.id = id;
  // Cast to non const is ok because we then take the whole chain as const
  buf->PrependChain(std::move(data));

  return connection_map_[to.ip].Then(
      MoveBind([](std::unique_ptr<IOBuf>&& data, SharedFuture<Connection*> f) {
                 f.Get()->Send(std::move(data));
                 f.Get()->Pcb().Output();
               },
               std::move(buf)));
}
