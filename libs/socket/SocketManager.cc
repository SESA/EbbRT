//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#include "SocketManager.h"
#include <ebbrt/Timer.h>

int ebbrt::SocketManager::NewIpv4Socket() {
  auto sfd = ebbrt::SocketManager::SocketFd::Create();
  return ebbrt::root_vfs->RegisterFd(sfd);
}

void ebbrt::SocketManager::SocketFd::TcpSession::Receive(
    std::unique_ptr<ebbrt::MutIOBuf> b) {
  {
    std::lock_guard<ebbrt::SpinLock> guard(buf_lock_);
    if (inbuf_) {
      inbuf_->PrependChain(std::move(b));
    } else {
      inbuf_ = std::move(b);
    }
  }  // unlocked buf_lock_
  check_read();
  return;
}

void ebbrt::SocketManager::SocketFd::TcpSession::Fire() {
  // TODO(jmc): check for additional states
  auto state = connected_.GetFuture();
  if (!state.Ready()) {
    connected_.SetValue(-1);
  }
}

void ebbrt::SocketManager::SocketFd::TcpSession::Connected() {
  connected_.SetValue(true);
  return;
}

void ebbrt::SocketManager::SocketFd::TcpSession::check_read() {

  std::lock_guard<ebbrt::SpinLock> guard(buf_lock_);
  // Confirm we have received new data or an unfulfilled read request
  if (!inbuf_ || !read_blocked_) {
    return;
  }

  ebbrt::kbugon(read_.first.GetFuture().Ready());
  auto message_len = read_.second;
  auto buffer_len = inbuf_->ComputeChainDataLength();

  if (likely(buffer_len == message_len)) {
    read_.first.SetValue(std::move(inbuf_));
    read_blocked_ = false;
    return;
  } else if (message_len == 0) {
    // return an empty IOBuf, keep any existing data in inbuf_
    read_.first.SetValue(ebbrt::MakeUniqueIOBuf(0));
    read_blocked_ = false;
    return;
  } else if (buffer_len < message_len) {
    // wait for more data to arrive
    return;
  } else if (buffer_len > message_len) {
    // extract and process the requested message 
    auto b = std::move(inbuf_);
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
      //  
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
    // split the buffer chain 
    auto left_shard_len = split->Length() - (length - message_len);
    auto right_shard_len = split->Length() - left_shard_len;
    auto split_c = IOBuf::Create<MutSharedIOBufRef>(SharedIOBufRef::CloneView,
                                                    std::move(split));
    auto remainder =
        IOBuf::Create<MutSharedIOBufRef>(SharedIOBufRef::CloneView, *split_c);
    split_c->TrimEnd(right_shard_len);
    remainder->Advance(left_shard_len);

    // combined peices of message into a IOBuf 
    if (!b) {
      b = std::move(split_c);
    } else {
      b->PrependChain(std::move(split_c));
    }
    if (tail_chain) {
      remainder->PrependChain(std::move(tail_chain));
    }

    kassert(buffer_len == (b->ComputeChainDataLength() +
                           remainder->ComputeChainDataLength()));
    kassert(message_len == b->ComputeChainDataLength());

    // return message buffer, store remainder
    inbuf_ = std::move(remainder);
    read_.first.SetValue(std::move(b));
    read_blocked_ = false;
    return;
  }
}

void ebbrt::SocketManager::SocketFd::TcpSession::Close() {
  read_.first.SetValue(ebbrt::MakeUniqueIOBuf(0));
  Shutdown();
  disconnected_.SetValue(0);
  return;
}

void ebbrt::SocketManager::SocketFd::TcpSession::Abort() {
  ebbrt::kabort("TCP Session Abort.\n");
  return;
}

ebbrt::Future<std::unique_ptr<ebbrt::IOBuf>>
ebbrt::SocketManager::SocketFd::Read(size_t len) {

  Promise<std::unique_ptr<ebbrt::IOBuf>> p;
  auto f = p.GetFuture();
  tcp_session_->read_blocked_ = true;
  tcp_session_->read_ = std::make_pair(std::move(p), len);
  tcp_session_->check_read();
  return std::move(f);
}

void ebbrt::SocketManager::SocketFd::Write(std::unique_ptr<IOBuf> buf) {
  tcp_session_->Send(std::move(buf));
  tcp_session_->Pcb().Output();
}

ebbrt::Future<uint8_t> ebbrt::SocketManager::SocketFd::Close() {
  tcp_session_->Shutdown();
  return tcp_session_->disconnected_.GetFuture();
}

void ebbrt::SocketManager::SocketFd::install_pcb(
    ebbrt::NetworkManager::TcpPcb pcb) {
  tcp_session_ = new TcpSession(this, std::move(pcb));
  tcp_session_->Install();
  return;
}

void ebbrt::SocketManager::SocketFd::check_waiting() {
  // only to be called while holding waiting_lock_ 
  while (!waiting_accept_.empty() && !waiting_pcb_.empty()) {

    auto a = std::move(waiting_accept_.front());
    auto pcb = std::move(waiting_pcb_.front());

    waiting_accept_.pop();
    waiting_pcb_.pop();

    auto s = ebbrt::socket_manager->NewIpv4Socket();
    auto fd = ebbrt::root_vfs->Lookup(s);
    static_cast<ebbrt::EbbRef<ebbrt::SocketManager::SocketFd>>(fd)->Connect(
        std::move(pcb));
    a.SetValue(s);
  }
}

ebbrt::Future<int> ebbrt::SocketManager::SocketFd::Accept() {
  std::lock_guard<ebbrt::SpinLock> guard(waiting_lock_);
  ebbrt::Promise<int> p;
  auto f = p.GetFuture();
  waiting_accept_.push(std::move(p));
  check_waiting();
  return f;
}

int ebbrt::SocketManager::SocketFd::Bind(uint16_t port) {
  // TODO(jmc): set state / error
  listen_port_ = port;
  return 0;
}

int ebbrt::SocketManager::SocketFd::Listen() {
  // TODO(jmc): set fd state
  try {
    listening_pcb_.Bind(
        listen_port_, [this](ebbrt::NetworkManager::TcpPcb pcb) {
          std::lock_guard<ebbrt::SpinLock> guard(waiting_lock_);
          ebbrt::kprintf("New connection arrived on listening socket.\n");
          waiting_pcb_.push(std::move(pcb));
          check_waiting();
        });
  } catch (std::exception& e) {
    // TODO(jmc): set errno
    ebbrt::kprintf("Unhandled exception caught: %s\n", e.what());
    return -1;
  } catch (...) {
    ebbrt::kprintf("Unhandled exception caught \n");
    return -1;
  }
  return 0;
}

bool ebbrt::SocketManager::SocketFd::ReadWouldBlock() {
  if (tcp_session_->read_blocked_) {
    ebbrt::kabort("warning: socket read already blocked \n");
    return false;
  }
  // no outstanding reads
  return (tcp_session_->inbuf_ == nullptr);
}

ebbrt::Future<uint8_t>
ebbrt::SocketManager::SocketFd::Connect(ebbrt::NetworkManager::TcpPcb pcb) {
  // TODO(jmc): check fd state
  install_pcb(std::move(pcb));
  return tcp_session_->connected_.GetFuture();
}

