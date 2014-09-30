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

void ebbrt::Messenger::Connection::Receive(std::unique_ptr<MutIOBuf> b) {
  // If we already have queued data append this new data to the end
  if (buf_) {
    buf_->PrependChain(std::move(b));
  } else {
    buf_ = std::move(b);
  }

  while (buf_) {
    auto chain_len = buf_->ComputeChainDataLength();
    // Do we have enough data for a header?
    if (chain_len < sizeof(Header))
      return;

    auto dp = buf_->GetDataPointer();
    auto& header = dp.Get<Header>();
    auto message_len = sizeof(Header) + header.length;

    std::unique_ptr<MutIOBuf> msg;
    if (likely(chain_len == message_len)) {
      // We have a full message
      msg = std::move(buf_);
    } else if (chain_len > message_len) {
      // After this loop msg should hold exactly one message and everything
      // else will be in buf_
      bool first = true;
      msg = std::move(buf_);
      for (auto& buf : *msg) {
        auto buf_len = buf.Length();
        if (buf_len == message_len) {
          // buf.Next() cannot be the head of the chain again because
          // chain_len > message_len
          buf_ = std::unique_ptr<MutIOBuf>(
              static_cast<MutIOBuf*>(msg->UnlinkEnd(*buf.Next()).release()));
          break;
        } else if (buf_len > message_len) {
          std::unique_ptr<MutIOBuf> end;
          if (first) {
            end = std::move(msg);
          } else {
            auto tmp_end =
                static_cast<MutIOBuf*>(msg->UnlinkEnd(buf).release());
            end = std::unique_ptr<MutIOBuf>(tmp_end);
          }

          auto remainder = end->Pop();

          // make a reference counted IOBuf to the end
          auto rc_end = IOBuf::Create<MutSharedIOBufRef>(std::move(end));
          // create a copy (increments ref count)
          buf_ = IOBuf::Create<MutSharedIOBufRef>(*rc_end);

          // trim and append to msg
          rc_end->TrimEnd(buf_len - message_len);
          if (first) {
            msg = std::move(rc_end);
          } else {
            msg->PrependChain(std::move(rc_end));
          }

          // advance and attach remainder to
          buf_->Advance(message_len);
          if (remainder)
            buf_->PrependChain(std::move(remainder));
          break;
        }
        message_len -= buf_len;
        first = false;
      }

    } else {
      return;
    }

    // msg now holds exactly one message
    // trim the header
    auto advance = sizeof(Header);
    for (auto& buf : *msg) {
      auto buf_len = buf.Length();
      buf.Advance(advance);
      if (buf_len >= advance)
        break;
      advance -= buf_len;
    }
    auto& ref = GetMessagableRef(header.id, header.type_code);
    ref.ReceiveMessageInternal(NetworkId(Pcb().GetRemoteAddress()),
                               std::move(msg));
  }
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
