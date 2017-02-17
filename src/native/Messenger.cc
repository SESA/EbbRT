//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Messenger.h"

#include "../Message.h"
#include "../SharedIOBufRef.h"
#include "../UniqueIOBuf.h"
#include "Debug.h"

uint16_t ebbrt::Messenger::port_;

ebbrt::Messenger::Messenger() {}

ebbrt::Messenger::Connection::Connection(ebbrt::NetworkManager::TcpPcb pcb)
    : TcpHandler(std::move(pcb)) {}

void ebbrt::Messenger::Connection::check_preallocate() {
  // preallocate buffer if payload occupancy ratio drops below threshold
  size_t capacity = 0;
  for (auto& buf : *buf_) {
    capacity += buf.Capacity();
  }
  auto buffer_len = buf_->ComputeChainDataLength();
  auto dp = buf_->GetDataPointer();
  auto& header = dp.Get<Header>();
  auto message_len = sizeof(Header) + header.length;
  auto ratio = static_cast<double>(buffer_len) / static_cast<double>(capacity);
  if (ratio < kOccupancyRatio) {
    // allocate message buffer and coalesce chain
    auto newbuf = MakeUniqueIOBuf(message_len, false);
    auto dp = newbuf->GetMutDataPointer();
    for (auto& buf : *buf_) {
      auto len = buf.Length();
      std::memcpy(static_cast<void*>(dp.Data()), buf.Data(), len);
      dp.Advance(len);
      preallocate_ += len;
    }
    assert(newbuf->CountChainElements() == 1);
    assert(newbuf->ComputeChainDataLength() == message_len);
    assert(preallocate_ == buffer_len);
    buf_ = std::move(newbuf);
  }
}

void ebbrt::Messenger::Connection::preallocated(std::unique_ptr<MutIOBuf> b) {
  auto dp = buf_->GetMutDataPointer();
  auto& header = dp.GetNoAdvance<Header>();
  auto message_len = sizeof(Header) + header.length;
  dp.Advance(preallocate_);
  for (auto& link : *b) {
    auto len = link.Length();
    std::memcpy(static_cast<void*>(dp.Data()), link.Data(), len);
    dp.Advance(len);
    preallocate_ += len;
  }
  if (preallocate_ == message_len) {
    kassert(buf_->Length() == message_len);
    preallocate_ = 0;
    process_message(std::move(buf_));
  }
  return;
}

void ebbrt::Messenger::Connection::process_message(
    std::unique_ptr<MutIOBuf> b) {
  auto dp = b->GetDataPointer();
  // TODO(dschatz): get rid of datapointer
  auto& header = dp.Get<Header>();
  b->AdvanceChain(sizeof(Header));
  auto& ref = GetMessagableRef(header.id, header.type_code);
  ref.ReceiveMessageInternal(NetworkId(Pcb().GetRemoteAddress()), std::move(b));
  return;
}
std::unique_ptr<ebbrt::MutIOBuf>
ebbrt::Messenger::Connection::process_message_chain(
    std::unique_ptr<MutIOBuf> b) {

  auto dp = b->GetDataPointer();
  auto& header = dp.Get<Header>();
  auto message_len = sizeof(Header) + header.length;

  // Our buffer contains the data of multiple messages. We need to
  // split the buffer at the first messsage boundary and preserve
  // the rest.
  auto orig_len = b->ComputeChainDataLength();
  std::unique_ptr<IOBuf> tail_chain;
  std::unique_ptr<IOBuf> split;
  uint32_t length = 0;
  // check if message is contained within first buffer
  if (b->Length() >= message_len) {
    length = b->Length();
    split = std::move(b);
    tail_chain = split->Pop();
    b = nullptr;
  } else {
    for (auto& buf : *b) {
      length += buf.Length();
      if (length >= message_len) {
        kassert(b->IsChained());
        auto tmp = static_cast<MutIOBuf*>(b->UnlinkEnd(buf).release());
        split = std::unique_ptr<MutIOBuf>(tmp);
        tail_chain = split->Pop();
        break;
      }
    }
  }
  // we "divide" the buffer by clone and resize
  auto left_shard_len = split->Length() - (length - message_len);
  auto right_shard_len = split->Length() - left_shard_len;
  auto split_c = IOBuf::Create<MutSharedIOBufRef>(SharedIOBufRef::CloneView,
                                                  std::move(split));
  auto remainder =
      IOBuf::Create<MutSharedIOBufRef>(SharedIOBufRef::CloneView, *split_c);
  split_c->TrimEnd(right_shard_len);
  remainder->Advance(left_shard_len);

  // combined data
  if (!b) {
    b = std::move(split_c);
  } else {
    b->PrependChain(std::move(split_c));
  }
  if (tail_chain) {
    remainder->PrependChain(std::move(tail_chain));
  }
  kassert(orig_len ==
          (b->ComputeChainDataLength() + remainder->ComputeChainDataLength()));

  process_message(std::move(b));
  return std::move(remainder);
}

void ebbrt::Messenger::Connection::Receive(std::unique_ptr<MutIOBuf> b) {
  kassert(b->Length() != 0);

  // check if we've preallocated a message buffer
  if (preallocate_) {
    preallocated(std::move(b));
    return;
  }
  // otherwise, process buffer chain
  if (buf_) {
    buf_->PrependChain(std::move(b));
  } else {
    buf_ = std::move(b);
  }

  while (buf_) {
    auto buffer_len = buf_->ComputeChainDataLength();
    if (buffer_len < sizeof(Header)) {
      return;
    }
    auto dp = buf_->GetDataPointer();
    auto& header = dp.Get<Header>();
    auto message_len = sizeof(Header) + header.length;
    if (likely(buffer_len == message_len)) {
      process_message(std::move(buf_));
      return;
    } else if (buffer_len < message_len) {
      // check if we need to preallocate
      // only  do this check at certain chain lengths
      if (buf_->CountChainElements() % kPreallocateChainLen == 0) {
        check_preallocate();
      }
      return;
    } else if (buffer_len > message_len) {
      // process message from chain and return remaining data
      buf_ = process_message_chain(std::move(buf_));
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

  return connection_map_[to.ip].Then([data = std::move(buf)](
      SharedFuture<Connection*> f) mutable {
    f.Get()->Send(std::move(data));
    f.Get()->Pcb().Output();
  });
}
