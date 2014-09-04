//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Net.h>

#include <ebbrt/IOBufRef.h>
#include <ebbrt/NetChecksum.h>
#include <ebbrt/Random.h>
#include <ebbrt/Timer.h>
#include <ebbrt/UniqueIOBuf.h>

// Destroy a listening tcp pcb
void ebbrt::NetworkManager::ListeningTcpPcb::ListeningTcpEntryDeleter::
operator()(ListeningTcpEntry* e) {
  kabort("UNIMPLEMENTED: ListeningTcpEntry Delete\n");
  // FIXME(dschatz): Port management is broken, this doesn't work
  if (e->port) {
    network_manager->tcp_port_allocator_->Free(e->port);
    std::lock_guard<std::mutex> guard(
        network_manager->listening_tcp_write_lock_);
    network_manager->listening_tcp_pcbs_.erase(*e);
  }
  event_manager->DoRcu([e]() { delete e; });
}

void ebbrt::NetworkManager::TcpPcb::TcpEntryDeleter::operator()(TcpEntry* e) {
  std::lock_guard<std::mutex> guard(network_manager->tcp_write_lock_);
  network_manager->tcp_pcbs_.erase(*e);
  // The TcpPcb doesn't delete the entry just yet, instead the entry will delete
  // itself once it has closed the connection with the remote side.
  e->Close();
}

// Bind a Listening PCB to a port (0 will be randomly assigned a new port).
// When a new connection is made, accept will be called. Return value is the
// port bound to
uint16_t ebbrt::NetworkManager::ListeningTcpPcb::Bind(
    uint16_t port, MovableFunction<void(TcpPcb)> accept) {
  if (!port) {
    auto ret = network_manager->tcp_port_allocator_->Allocate();
    if (!ret)
      throw std::runtime_error("Failed to allocate ephemeral port");

    port = *ret;
  }
  // TODO(dschatz): else, check port is free and mark it as allocated

  entry_->port = port;
  entry_->accept_fn = std::move(accept);
  {
    // ensure that all mutating operations on the hash table are serialized
    std::lock_guard<std::mutex> guard(
        network_manager->listening_tcp_write_lock_);
    network_manager->listening_tcp_pcbs_.insert(*entry_);
  }
  return port;
}

// Install a handler for TCP connection events (receive packet, window size
// change, etc.)
void ebbrt::NetworkManager::TcpPcb::InstallHandler(
    std::unique_ptr<ITcpHandler> handler) {
  entry_->handler = std::move(handler);
}

// How large is the remote window?
uint16_t ebbrt::NetworkManager::TcpPcb::RemoteWindowRemaining() {
  return entry_->WindowRemaining();
}

// Enable/Disable window change notifications
void ebbrt::NetworkManager::TcpPcb::SetWindowNotify(bool notify) {
  entry_->window_notify = notify;
}

// Send TCP data on a connection. The user must ensure that the remote
// window is large enough as the PCB will do no buffering
void ebbrt::NetworkManager::TcpPcb::Send(std::unique_ptr<IOBuf> buf) {
  kassert(buf->ComputeChainDataLength() <= RemoteWindowRemaining());

  entry_->Send(std::move(buf));
}

// Receive a TCP packet on an interface
void
ebbrt::NetworkManager::Interface::ReceiveTcp(const Ipv4Header& ih,
                                             std::unique_ptr<MutIOBuf> buf) {
  auto packet_len = buf->ComputeChainDataLength();

  // Ensure we have a header
  if (unlikely(packet_len < sizeof(TcpHeader)))
    return;

  auto dp = buf->GetDataPointer();
  const auto& tcp_header = dp.Get<TcpHeader>();

  // drop broadcast/multicast
  auto addr = Address();
  if (unlikely(addr->isBroadcast(ih.dst) || ih.dst.isMulticast()))
    return;

  if (unlikely(IpPseudoCsum(*buf, ih.proto, ih.src, ih.dst)))
    return;

  // salient info for a tcp packet which we reuse throughout the process
  TcpInfo info = {.src_port = ntohs(tcp_header.src_port),
                  .dst_port = ntohs(tcp_header.dst_port),
                  .seqno = ntohl(tcp_header.seqno),
                  .ackno = ntohl(tcp_header.ackno),
                  .tcplen =
                      packet_len - tcp_header.HdrLen() +
                      ((tcp_header.Flags() & (kTcpFin | kTcpSyn)) ? 1 : 0)};

  // Only SYN flag set? check listening pcbs
  auto reset = false;
  if ((tcp_header.Flags() & (kTcpSyn | kTcpAck)) == kTcpSyn) {
    auto entry =
        network_manager->listening_tcp_pcbs_.find(ntohs(tcp_header.dst_port));

    if (!entry) {
      reset = true;
    } else {
      entry->Input(ih, tcp_header, info, std::move(buf));
    }
  } else {
    // Otherwise, check connected pcbs
    auto key = std::make_tuple(ih.src, info.src_port, info.dst_port);
    auto entry = network_manager->tcp_pcbs_.find(key);
    if (!entry) {
      // no entry found for this packet, send reset
      reset = true;
    } else {
      entry->Input(ih, tcp_header, info, std::move(buf));
    }
  }

  if (reset) {
    network_manager->TcpReset(info.ackno, info.seqno + info.tcplen, ih.dst,
                              ih.src, info.dst_port, info.src_port);
  }
}

// Timer fires
void ebbrt::NetworkManager::TcpEntry::Fire() {
  timer_set = false;
  // We take a single clock reading which we use to simplify some corner cases
  // with respect to enabling the timer. This way there is a single time point
  // when this event occurred and all clock computations can be relative to it.
  auto now = ebbrt::clock::Wall::Now();
  // If we have a retransmit time and we have passed it, disable retransmit
  // timer and move all unacked segments to pending
  if (retransmit != ebbrt::clock::Wall::time_point() && now >= retransmit) {
    retransmit = ebbrt::clock::Wall::time_point();
    // Move all unacked segments to the front of the pending segments queue
    pending_segments.splice(pending_segments.begin(),
                            std::move(unacked_segments));
  }

  // Try to send what we can
  Output(now);
  // Set the timer if we have to
  SetTimer(now);
}

// Set the timer if it is not already set and we have the need to
void
ebbrt::NetworkManager::TcpEntry::SetTimer(ebbrt::clock::Wall::time_point now) {
  if (timer_set)
    return;

  // For now we only have a retransmit timer, if it needs to be set, we set it
  if (now >= retransmit)
    return;

  auto min_timer = retransmit;
  timer->Start(*this, std::chrono::duration_cast<std::chrono::microseconds>(
                          min_timer - now),
               /* repeat = */ false);
  timer_set = true;
}

// Input tcp segment to a listening PCB
void ebbrt::NetworkManager::ListeningTcpEntry::Input(
    const Ipv4Header& ih, const TcpHeader& th, const TcpInfo& info,
    std::unique_ptr<MutIOBuf> buf) {
  auto flags = th.Flags();

  if (flags & kTcpRst)
    return;

  if (flags & kTcpAck) {
    // We shouldn't receive anything but a SYN packet, send a reset
    network_manager->TcpReset(info.ackno, info.seqno + info.tcplen, ih.dst,
                              ih.src, info.dst_port, info.src_port);
  } else if (flags & kTcpSyn) {
    // Create a new tcp entry for the connection
    auto entry = new TcpEntry(&accept_fn);

    entry->address = ih.dst;
    std::get<0>(entry->key) = ih.src;
    std::get<1>(entry->key) = info.src_port;
    std::get<2>(entry->key) = info.dst_port;
    entry->state = TcpEntry::State::kSynReceived;
    uint32_t start_seq = random::Get();
    entry->snd_lbb = start_seq;
    entry->rcv_nxt = info.seqno + info.tcplen;
    entry->lastack = start_seq;
    entry->rcv_wnd = ntohs(th.wnd);

    // Create a SYN-ACK reply
    auto buf = std::unique_ptr<MutUniqueIOBuf>(new MutUniqueIOBuf(
        sizeof(TcpHeader) + sizeof(Ipv4Header) + sizeof(EthernetHeader)));
    buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));
    auto dp = buf->GetMutDataPointer();
    auto& tcp_header = dp.Get<TcpHeader>();
    entry->EnqueueSegment(tcp_header, std::move(buf), kTcpSyn | kTcpAck);

    {
      // ensure that all mutating operations on the hash table are serialized
      std::lock_guard<std::mutex> lock(network_manager->tcp_write_lock_);
      // double check that we haven't concurrently created this connection
      auto found_entry = network_manager->tcp_pcbs_.find(entry->key);
      if (unlikely(found_entry)) {
        delete entry;
        kabort("UNIMPLEMENTED: Concurrent connection creation!\n");
      }

      network_manager->tcp_pcbs_.insert(*entry);
    }

    auto now = ebbrt::clock::Wall::Now();
    entry->Output(now);
    entry->SetTimer(now);
  }
}

// TCP Sequence computations
namespace {
bool TcpSeqBetween(uint32_t in, uint32_t left, uint32_t right) {
  return ((right - left) >= (in - left));
}

bool TcpSeqLT(uint32_t first, uint32_t second) {
  return ((int32_t)(first - second)) < 0;
}

bool TcpSeqGT(uint32_t first, uint32_t second) {
  return ((int32_t)(first - second)) < 0;
}
}  // namespace

ebbrt::NetworkManager::TcpEntry::TcpEntry(MovableFunction<void(TcpPcb)>* accept)
    : accept_fn(accept) {}

// Send on a TCP connection
void ebbrt::NetworkManager::TcpEntry::Send(std::unique_ptr<IOBuf> buf) {
  // Prepend a header to the chain which will Ack any received data
  auto header_buf = std::unique_ptr<MutUniqueIOBuf>(new MutUniqueIOBuf(
      sizeof(TcpHeader) + sizeof(Ipv4Header) + sizeof(EthernetHeader)));
  header_buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));
  header_buf->PrependChain(std::move(buf));
  auto dp = header_buf->GetMutDataPointer();
  auto& tcp_header = dp.Get<TcpHeader>();
  EnqueueSegment(tcp_header, std::move(header_buf), kTcpAck);
}

// Input on a TCP connection
void ebbrt::NetworkManager::TcpEntry::Input(const Ipv4Header& ih,
                                            const TcpHeader& th,
                                            const TcpInfo& info,
                                            std::unique_ptr<MutIOBuf> buf) {
  auto flags = th.Flags();

  if (flags & kTcpRst)
    return;

  auto reset = false;
  switch (state) {
  case kSynReceived:
    if (flags & kTcpAck) {
      if (TcpSeqBetween(info.ackno, lastack + 1, snd_lbb)) {
        // This is an ACK for the SYN-ACK, the three-way handshake is complete
        state = kEstablished;
        kassert(accept_fn != nullptr);
        (*accept_fn)(TcpPcb(this));
        // TODO(dschatz): the accept function could bind this connection to a
        // new core
        // we should SpawnRemote
        Receive(ih, th, info, std::move(buf));
      } else {
        reset = true;
      }
    } else if ((flags & kTcpSyn) && info.seqno == rcv_nxt - 1) {
      kabort("UNIMPLEMENTED: Extra SYN, need to retransmit SYN-ACK");
    }
    break;
  case kCloseWait:
  case kEstablished:
    Receive(ih, th, info, std::move(buf));
    break;
  case kLastAck:
    Receive(ih, th, info, std::move(buf));
    if (flags & kTcpAck && info.ackno == snd_lbb) {
      // All outstanding data is acked, clean up the connection
      state = kClosed;
      if (timer_set)
        timer->Stop(*this);

      {
        std::lock_guard<std::mutex> guard(network_manager->tcp_write_lock_);
        network_manager->tcp_pcbs_.erase(*this);
      }
      event_manager->DoRcu([this]() { delete this; });
    }
    break;
  default:
    kabort("UNIMPLEMENTED: Input State\n");
  }

  if (reset) {
    kabort("UNIMPLEMENTED: Reset on input\n");
  }

  auto now = ebbrt::clock::Wall::Now();
  Output(now);
  SetTimer(now);
}

// How much of the local receive window remains open
uint16_t ebbrt::NetworkManager::TcpEntry::WindowRemaining() {
  int32_t remaining = rcv_wnd - (snd_lbb - lastack);
  return remaining >= 0 ? remaining : 0;
}

// Receive a segment on a connection
void ebbrt::NetworkManager::TcpEntry::Receive(const Ipv4Header& ih,
                                              const TcpHeader& th,
                                              const TcpInfo& info,
                                              std::unique_ptr<MutIOBuf> buf) {
  auto flags = th.Flags();

  rcv_wnd = ntohs(th.wnd);

  // ACK processing
  if (flags & kTcpAck) {
    if (TcpSeqLT(lastack, info.ackno)) {
      // Ack for unacked data
      lastack = info.ackno;

      auto clear_acked_segments = [&](
          boost::container::list<TcpSegment>& queue) {
        auto it = queue.begin();
        while (it != queue.end()) {
          if (TcpSeqGT(ntohl(it->th.seqno) + it->SeqLen(), info.ackno))
            break;
          auto prev_it = it;
          ++it;
          queue.erase(prev_it);
        }
      };

      // Remove all unacked segments that have been completely acked by this ACK
      clear_acked_segments(unacked_segments);
      // Its also possible to find pending segments which have been ACKed
      // because we may have hit a retransmit timer which would cause them to be
      // moved back to the pending_segments queue. Remove them and
      clear_acked_segments(pending_segments);

      if (unacked_segments.empty()) {
        // disable retransmit timer
        retransmit = ebbrt::clock::Wall::time_point();
      }
      // Notify handler of window increase
      if (window_notify)
        handler->WindowIncrease(WindowRemaining());
    }
  }

  // Data processing only if theres data and we are not in the CloseWait state
  if (info.tcplen > 0 && state < kCloseWait) {
    if (TcpSeqBetween(rcv_nxt, info.seqno, info.seqno + info.tcplen - 1)) {
      buf->Advance(rcv_nxt - info.seqno);
      // TODO(dschatz): This could overrun our local receive window, we should
      // do something in that case like drop part of the data

      rcv_nxt = info.seqno + info.tcplen;

      // Upcall user application
      buf->Advance(sizeof(TcpHeader));
      handler->Receive(std::move(buf));

      if (flags & kTcpFin) {
        state = kCloseWait;
        handler->Receive(nullptr);
      }

    } else if (TcpSeqLT(info.seqno, rcv_nxt)) {
      kabort("UNIMPLEMENTED: Empty Ack\n");
    } else {
      // Out of sequence
      kabort("UNIMPLEMENTED: OOS: Empty Ack\n");
    }
  } else {
    if (!TcpSeqBetween(info.seqno, rcv_nxt, rcv_nxt + rcv_wnd - 1)) {
      kabort("UNIMPLEMENTED: empty ack, may need to ack back\n");
    }
  }
}

// Fill out a header and enqueue the segment to be sent
void ebbrt::NetworkManager::TcpEntry::EnqueueSegment(
    TcpHeader& th, std::unique_ptr<MutIOBuf> buf, uint16_t flags) {
  auto packet_len = buf->ComputeChainDataLength();
  auto tcp_len =
      packet_len - sizeof(TcpHeader) + ((flags & (kTcpSyn | kTcpFin)) ? 1 : 0);
  th.src_port = htons(std::get<2>(key));
  th.dst_port = htons(std::get<1>(key));
  th.seqno = htonl(snd_lbb);
  th.SetHdrLenFlags(sizeof(TcpHeader), flags);
  // ackno, wnd, and checksum are set in Output()
  th.urgp = 0;

  pending_segments.emplace_back(std::move(buf), th, tcp_len);

  snd_lbb += tcp_len;
}

// Attempt to send any outstanding tcp segments
size_t
ebbrt::NetworkManager::TcpEntry::Output(ebbrt::clock::Wall::time_point now) {
  auto it = pending_segments.begin();

  auto wnd = std::min(kTcpCWnd, rcv_wnd);

  // try to send as many pending segments as will fit in the window
  size_t sent = 0;
  for (; it != pending_segments.end() &&
             ntohl(it->th.seqno) - lastack + it->tcp_len <= wnd;
       ++it) {
    SendSegment(*it);
    ++sent;
  }

  // If we sent some segments, add them to the unacked list
  if (sent) {
    unacked_segments.splice(unacked_segments.end(), std::move(pending_segments),
                            pending_segments.begin(), it, sent);
  } else {
    if (!pending_segments.empty() && unacked_segments.empty()) {
      // If we have no outstanding segments to be acked and there is at least
      // one segment pending, we need either:
      if (wnd > 0) {
        // Shrink the front most segment so it will fit in the window
        kabort("UNIMPLEMENTED: Shrink pending segment to be sent\n");
      } else {
        // Set a persist timer to periodically probe for window size changes
        kabort("UNIMPLEMENTED: Window size probe needed\n");
      }
    }

    // In the case that we don't have any data to send out but we have received
    // data since our last ACK, we will send an empty ACK
    if (TcpSeqLT(last_sent_ack, rcv_nxt)) {
      SendEmptyAck();
    }
  }

  if (sent) {
    retransmit = now + std::chrono::milliseconds(250);
  }

  return sent;
}

// Send an Ack with no data
void ebbrt::NetworkManager::TcpEntry::SendEmptyAck() {
  auto buf = std::unique_ptr<MutUniqueIOBuf>(new MutUniqueIOBuf(
      sizeof(TcpHeader) + sizeof(Ipv4Header) + sizeof(EthernetHeader)));
  buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));
  auto dp = buf->GetMutDataPointer();
  auto& th = dp.Get<TcpHeader>();
  th.src_port = htons(std::get<2>(key));
  th.dst_port = htons(std::get<1>(key));
  th.seqno = htonl(snd_lbb);
  th.SetHdrLenFlags(sizeof(TcpHeader), kTcpAck);
  th.urgp = 0;
  last_sent_ack = rcv_nxt;
  th.ackno = htonl(rcv_nxt);
  th.wnd = htons(kTcpWnd);
  th.checksum = 0;
  th.checksum = IpPseudoCsum(*buf, kIpProtoTCP, address, std::get<0>(key));

  network_manager->SendIp(std::move(buf), address, std::get<0>(key),
                          kIpProtoTCP);
}

// When Close() is called we will send a FIN and wait for all outstanding
// segments to be acked before deleting the entry
void ebbrt::NetworkManager::TcpEntry::Close() {
  switch (state) {
  case kCloseWait:
    SendFin();
    state = kLastAck;
    break;
  default:
    kabort("UNIMPLEMENTED: Close()\n");
  }
}

// Send a fin
void ebbrt::NetworkManager::TcpEntry::SendFin() {
  // Try to add fin to the last segment on the pending queue
  if (!pending_segments.empty()) {
    auto& seg = pending_segments.back();
    auto flags = seg.th.Flags();
    if (!(flags & (kTcpSyn | kTcpFin | kTcpRst))) {
      // no syn/fin/rst set
      seg.th.SetHdrLenFlags(sizeof(TcpHeader), flags | kTcpFin);
      ++seg.tcp_len;
      ++snd_lbb;
      return;
    }
  }

  // Otherwise create an empty segment with a Fin
  auto buf = std::unique_ptr<MutUniqueIOBuf>(new MutUniqueIOBuf(
      sizeof(TcpHeader) + sizeof(Ipv4Header) + sizeof(EthernetHeader)));
  buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));
  auto dp = buf->GetMutDataPointer();
  auto& tcp_header = dp.Get<TcpHeader>();
  EnqueueSegment(tcp_header, std::move(buf), kTcpFin | kTcpAck);
}

// Actually send a segment via IP
void ebbrt::NetworkManager::TcpEntry::SendSegment(TcpSegment& segment) {
  last_sent_ack = rcv_nxt;
  segment.th.ackno = htonl(rcv_nxt);
  segment.th.wnd = htons(kTcpWnd);
  segment.th.checksum = 0;
  segment.th.checksum =
      IpPseudoCsum(*(segment.buf), kIpProtoTCP, address, std::get<0>(key));

  network_manager->SendIp(CreateRefChain(*(segment.buf)), address,
                          std::get<0>(key), kIpProtoTCP);
}

// Send a reset packet
void ebbrt::NetworkManager::TcpReset(uint32_t seqno, uint32_t ackno,
                                     const Ipv4Address& local_ip,
                                     const Ipv4Address& remote_ip,
                                     uint16_t local_port,
                                     uint16_t remote_port) {
  auto buf = std::unique_ptr<MutUniqueIOBuf>(new MutUniqueIOBuf(
      sizeof(TcpHeader) + sizeof(Ipv4Header) + sizeof(EthernetHeader)));

  buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));

  auto dp = buf->GetMutDataPointer();
  auto& tcp_header = dp.Get<TcpHeader>();

  tcp_header.src_port = htons(local_port);
  tcp_header.dst_port = htons(remote_port);
  tcp_header.seqno = htonl(seqno);
  tcp_header.ackno = htonl(ackno);
  tcp_header.SetHdrLenFlags(sizeof(TcpHeader), kTcpRst | kTcpAck);
  tcp_header.wnd = htons(kTcpWnd);
  tcp_header.checksum = 0;
  tcp_header.urgp = 0;
  tcp_header.checksum = IpPseudoCsum(*buf, kIpProtoTCP, local_ip, remote_ip);

  SendIp(std::move(buf), local_ip, remote_ip, kIpProtoTCP);
}
