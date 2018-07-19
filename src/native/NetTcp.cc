//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Net.h"

#include "../IOBufRef.h"
#include "../Timer.h"
#include "../UniqueIOBuf.h"
#include "NetChecksum.h"
#include "Random.h"

// Destroy a listening tcp pcb
void ebbrt::NetworkManager::ListeningTcpPcb::ListeningTcpEntryDeleter::
operator()(ListeningTcpEntry* e) {
  if (e->port) {
    network_manager->tcp_port_allocator_->Free(e->port);
    std::lock_guard<ebbrt::SpinLock> guard(
        network_manager->listening_tcp_write_lock_);
    network_manager->listening_tcp_pcbs_.erase(*e);
  }
  event_manager->DoRcu([e]() { delete e; });
}

// Destroy a connected tcp pcb
void ebbrt::NetworkManager::TcpPcb::TcpEntryDeleter::operator()(TcpEntry* e) {
  // The TcpPcb doesn't delete the entry just yet, instead the entry will delete
  // itself once it has closed the connection with the remote side.
  e->Close();
  auto now = ebbrt::clock::Wall::Now();
  e->Output(now);
  e->SetTimer(now);
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
  } else if (port >= 49152 &&
             !network_manager->tcp_port_allocator_->Reserve(port)) {
    throw std::runtime_error("Failed to reserve specified port");
  }

  entry_->port = port;
  entry_->accept_fn = std::move(accept);
  {
    // ensure that all mutating operations on the hash table are serialized
    std::lock_guard<ebbrt::SpinLock> guard(
        network_manager->listening_tcp_write_lock_);
    network_manager->listening_tcp_pcbs_.insert(*entry_);
  }
  return port;
}

uint16_t ebbrt::NetworkManager::TcpPcb::Connect(Ipv4Address address,
                                                uint16_t port,
                                                uint16_t local_port) {
  // ensure we have an interface to send to this IP
  auto itf = network_manager->IpRoute(address);
  if (!itf) {
    throw std::runtime_error("No route to remote address for Connect");
  }

  if (!local_port) {
    auto ret = network_manager->tcp_port_allocator_->Allocate();
    if (!ret)
      throw std::runtime_error("Failed to allocate ephemeral port");

    local_port = *ret;
  } else if (local_port >= 49152 &&
             !network_manager->tcp_port_allocator_->Reserve(local_port)) {
    throw std::runtime_error("Failed to reserve specified port");
  }

  // Setup state
  entry_->accepted = true;
  entry_->address = itf->Address()->address;
  std::get<0>(entry_->key) = address;
  std::get<1>(entry_->key) = port;
  std::get<2>(entry_->key) = local_port;
  entry_->state = TcpEntry::State::kSynSent;
  uint32_t iss = random::Get();
  entry_->snd_una = iss;
  entry_->snd_nxt = iss;  // EnqueueSegment will increment this by one
  // We should wait to hear back from our Syn before setting this
  entry_->snd_wnd = kTcpWnd;
  entry_->rcv_nxt = 0;
  entry_->rcv_wnd = kTcpWnd;

  // We need to insert the entry into the hash table at this point to avoid
  // concurrent connection creation.
  // TODO(dschatz): In order for this to be safe in the face of concurrency,
  // we need to mark the entry as invalid so that data received on other cores
  // do not try to concurrently access the entry
  {
    // ensure that all mutating operations on the hash table are serialized
    std::lock_guard<ebbrt::SpinLock> lock(network_manager->tcp_write_lock_);
    // double check that we haven't concurrently created this connection
    auto found_entry = network_manager->tcp_pcbs_.find(entry_->key);
    if (unlikely(found_entry)) {
      // Concurrent SYNs raced and this one lost
      // Delete the new entry and pass the packet along to the active one
      throw std::runtime_error("Connection already created");
    }

    network_manager->tcp_pcbs_.insert(*entry_);
  }

  // TODO(dschatz): There should be a timeout to close the new connection if
  // the handshake doesn't complete

  auto optlen = 8;  // for MSS + NOP + WS
  auto new_buf = MakeUniqueIOBuf(optlen + sizeof(TcpHeader) +
                                 sizeof(Ipv4Header) + sizeof(EthernetHeader));
  new_buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));
  auto dp = new_buf->GetMutDataPointer();
  auto& tcp_header = dp.Get<TcpHeader>();
  auto opts = reinterpret_cast<uint32_t*>((&tcp_header) + 1);
  *opts = htonl(0x02040000 | (1460 & 0xFFFF));
  auto nop_ptr = reinterpret_cast<uint8_t*>(opts+1);
  nop_ptr[0] = 0x1; // NOP
  nop_ptr[1] = 0x3; // WS type
  nop_ptr[2] = 0x3; // opt length
  nop_ptr[3] = kWindowShift; // shift value
  entry_->EnqueueSegment(tcp_header, std::move(new_buf), kTcpSyn, optlen);

  auto now = ebbrt::clock::Wall::Now();
  entry_->Output(now);
  entry_->SetTimer(now);

  return local_port;
}

void ebbrt::NetworkManager::TcpPcb::OpenWindow() {
  entry_->close_window = false;
  entry_->rcv_wnd = kTcpWnd;
}

void ebbrt::NetworkManager::TcpPcb::CloseWindow() {
  entry_->close_window = true;
}

// Bind this connection to a core
void ebbrt::NetworkManager::TcpPcb::BindCpu(size_t index) {
  entry_->cpu = index;
}

// Install a handler for TCP connection events (receive packet, window size
// change, etc.)
void ebbrt::NetworkManager::TcpPcb::InstallHandler(
    std::unique_ptr<ITcpHandler> handler) {
  entry_->handler = std::move(handler);
}

// How large is the remote window?
size_t ebbrt::NetworkManager::TcpPcb::SendWindowRemaining() {
  return entry_->SendWindowRemaining();
}

// Enable/Disable window change notifications
void ebbrt::NetworkManager::TcpPcb::SetWindowNotify(bool notify) {
  entry_->window_notify = notify;
}

// Send TCP data on a connection. The user must ensure that the remote
// window is large enough as the PCB will do no buffering
void ebbrt::NetworkManager::TcpPcb::Send(std::unique_ptr<IOBuf> buf) {
  kassert(buf->ComputeChainDataLength() <= SendWindowRemaining());
  const constexpr size_t max_ipv4_header_size = 60;
  const constexpr size_t max_tcp_header_size = 60;
  const constexpr size_t max_ipv4_packet_size = UINT16_MAX;
  const constexpr size_t max_tcp_segment_size =
      max_ipv4_packet_size - max_tcp_header_size - max_ipv4_header_size;
  kassert(buf->ComputeChainDataLength() <= max_tcp_segment_size);

  entry_->Send(std::move(buf));
}

void ebbrt::NetworkManager::TcpPcb::Output() {
  auto now = ebbrt::clock::Wall::Now();
  entry_->Output(now);
  entry_->SetTimer(now);
}

ebbrt::Ipv4Address ebbrt::NetworkManager::TcpPcb::GetRemoteAddress() {
  return std::get<0>(entry_->key);
}

// Receive a TCP packet on an interface
void ebbrt::NetworkManager::Interface::ReceiveTcp(const Ipv4Header& ih,
                                                  std::unique_ptr<MutIOBuf> buf,
                                                  uint64_t rxflag) {
  auto packet_len = buf->ComputeChainDataLength();

  // Ensure we have a header
  if (unlikely(packet_len < sizeof(TcpHeader)))
    return;

  auto dp = buf->GetMutDataPointer();
  auto& tcp_header = dp.Get<TcpHeader>();

  // drop broadcast/multicast
  auto addr = Address();
  if (unlikely(addr->isBroadcast(ih.dst) || ih.dst.isMulticast()))
    return;

// XXX: Check if rxcsum is supported
// if (unlikely(IpPseudoCsum(*buf, ih.proto, ih.src, ih.dst)))
//   return;

#ifdef __EBBRT_ENABLE_BAREMETAL_NIC__
  if (unlikely((rxflag & RXFLAG_L4CS) == 0)) {
    ebbrt::kprintf("%s RXFLAG_L4CS failed\n");
    return;
  }

  if (unlikely((rxflag & RXFLAG_L4CS_VALID) == 0)) {
    ebbrt::kprintf("%s RXFLAG_L4CS_VALID failed\n");
    return;
  }
#endif
  auto hdr_len = tcp_header.HdrLen();
  if (unlikely(hdr_len < sizeof(TcpHeader) || hdr_len > packet_len))
    return;

  // salient info for a tcp packet which we reuse throughout the process
  TcpInfo info = {.src_port = ntohs(tcp_header.src_port),
                  .dst_port = ntohs(tcp_header.dst_port),
                  .seqno = ntohl(tcp_header.seqno),
                  .ackno = ntohl(tcp_header.ackno),
                  .tcplen =
                      packet_len - hdr_len +
                      ((tcp_header.Flags() & (kTcpFin | kTcpSyn)) ? 1 : 0)};

  // Check connected pcbs
  auto key = std::make_tuple(ih.src, info.src_port, info.dst_port);
  auto entry = network_manager->tcp_pcbs_.find(key);
  if (entry) {
    kbugon(!entry->accepted,
           "User's accept() call hasn't completed before more data arrived\n");
    if (entry->cpu == Cpu::GetMine()) {
      entry->Input(ih, tcp_header, info, std::move(buf));
    } else {
      // XXX: Really nervous about passing these references, but I think its all
      // safe, for now
      auto f = [
        &ih, &tcp_header, entry, buf = std::move(buf), info = std::move(info)
      ]() mutable {
        entry->Input(ih, tcp_header, info, std::move(buf));
      };
      event_manager->SpawnRemote(std::move(f), entry->cpu);
    }
  } else {
    // If no connection found, check listening pcbs
    auto entry =
        network_manager->listening_tcp_pcbs_.find(ntohs(tcp_header.dst_port));

    if (likely(entry)) {
      entry->Input(ih, tcp_header, info, std::move(buf));
    } else if (!(tcp_header.Flags() & kTcpRst)) {
      // RFC 793 page 65
      // If the state is CLOSED (i.e., TCB does not exist) then all data in the
      // incoming segment is discarded.  An incoming segment containing a RST is
      // discarded.  An incoming segment not containing a RST causes a RST to be
      // sent in response.  The acknowledgment and sequence field values are
      // selected to make the reset sequence acceptable to the TCP that sent the
      // offending segment.

      // If the ACK bit is off, sequence number zero is used,
      //   <SEQ=0><ACK=SEG.SEQ+SEG.LEN><CTL=RST,ACK>
      // If the ACK bit is on,
      //   <SEQ=SEG.ACK><CTL=RST>
      auto ack = tcp_header.Flags() & kTcpAck;
      auto seqno = ack ? info.ackno : 0;
      network_manager->TcpReset(!ack, seqno, info.seqno + info.tcplen, ih.dst,
                                ih.src, info.dst_port, info.src_port);
    }
  }
}

// Timer fires
void ebbrt::NetworkManager::TcpEntry::Fire() {
  timer_set = false;
  // We take a single clock reading which we use to simplify some corner cases
  // with respect to enabling the timer. This way there is a single time point
  // when this event occurred and all clock computations can be relative to it.
  auto now = ebbrt::clock::Wall::Now();

  // If we reached the time_wait period then destroy the pcb
  if (time_wait != ebbrt::clock::Wall::time_point() && now >= time_wait) {
    time_wait = ebbrt::clock::Wall::time_point();

    Purge();
    DisableTimers();
    Destroy();
    return;
  }

  // If we have a retransmit timeout and we have passed it, disable retransmit
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
void ebbrt::NetworkManager::TcpEntry::SetTimer(
    ebbrt::clock::Wall::time_point now) {
  if (timer_set)
    return;

  ebbrt::clock::Wall::time_point min_timer;
  if (now >= retransmit && now >= time_wait) {
    return;
  } else if (now >= retransmit) {
    min_timer = time_wait;
  } else {
    min_timer = retransmit;
  }

  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(min_timer - now);
  timer->Start(*this, duration, /* repeat = */ false);
  timer_set = true;
}

// Turn off all timers
void ebbrt::NetworkManager::TcpEntry::DisableTimers() {
  if (timer_set) {
    timer->Stop(*this);
  }

  timer_set = false;

  retransmit = ebbrt::clock::Wall::time_point();
  time_wait = ebbrt::clock::Wall::time_point();
}

// Purge all outstanding segments (either pending or unacked)
void ebbrt::NetworkManager::TcpEntry::Purge() {
  unacked_segments.clear();
  pending_segments.clear();
}

// Remove a pcb from its list and destroy it
void ebbrt::NetworkManager::TcpEntry::Destroy() {
  {
    std::lock_guard<ebbrt::SpinLock> guard(network_manager->tcp_write_lock_);
    network_manager->tcp_pcbs_.erase(*this);
  }
  event_manager->DoRcu([this]() { delete this; });
}

// Input tcp segment to a listening PCB
void ebbrt::NetworkManager::ListeningTcpEntry::Input(
    const Ipv4Header& ih, TcpHeader& th, TcpInfo& info,
    std::unique_ptr<MutIOBuf> buf) {
  auto flags = th.Flags();

  if (flags & kTcpRst)
    // RFC 793 page 65: If the state is LISTEN then first check for an RST
    //   An incoming RST should be ignored."
    return;

  if (flags & kTcpAck) {
    // RFC 793 page 65: "Any acknowledgment is bad if it arrives on a
    // connection still in the LISTEN state.  An acceptable reset
    // segment should be formed for any arriving ACK-bearing segment.
    // The RST should be formatted as follows:
    // <SEQ=SEG.ACK><CTL=RST>"
    network_manager->TcpReset(false, info.ackno, 0, ih.dst, ih.src,
                              info.dst_port, info.src_port);
  } else if (flags == kTcpSyn) {  // Only SYN flag is set
    // New connection is being created in the SynReceived state

    // Create a new tcp entry for the connection
    auto entry = new TcpEntry();

    // Setup entry initial state
    entry->cpu = Cpu::GetMine();
    entry->address = ih.dst;
    std::get<0>(entry->key) = ih.src;
    std::get<1>(entry->key) = info.src_port;
    std::get<2>(entry->key) = info.dst_port;
    entry->state = TcpEntry::State::kSynReceived;
    uint32_t start_seq = random::Get();
    entry->snd_nxt = start_seq;  // EnqueueSegment will increment this by one

    // We need to insert the entry into the hash table at this point to avoid
    // concurrent connection creation.
    {
      // ensure that all mutating operations on the hash table are serialized
      std::lock_guard<ebbrt::SpinLock> lock(network_manager->tcp_write_lock_);
      // double check that we haven't concurrently created this connection
      auto found_entry = network_manager->tcp_pcbs_.find(entry->key);
      if (unlikely(found_entry)) {
        // Concurrent SYNs raced and this one lost
        // Delete the new entry and pass the packet along to the active one
        delete entry;
        found_entry->Input(ih, th, info, std::move(buf));
        return;
      }

      network_manager->tcp_pcbs_.insert(*entry);
    }

    // TODO(dschatz): There should be a timeout to close the new connection if
    // the handshake doesn't complete

    // We have received the SYN flag, so mark that byte as received
    entry->rcv_nxt = info.seqno + 1;

    entry->snd_una = start_seq;
    // TODO(dschatz): Read the window shift from the options!!!
    entry->snd_wnd = ntohs(th.wnd) << kWindowShift;
    entry->rcv_wnd = kTcpWnd;

    // Create a SYN-ACK reply
    auto optlen = 8;  // for MSS + NOP + WS
    auto new_buf = MakeUniqueIOBuf(optlen + sizeof(TcpHeader) +
                                   sizeof(Ipv4Header) + sizeof(EthernetHeader));
    new_buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));
    auto dp = new_buf->GetMutDataPointer();
    auto& tcp_header = dp.Get<TcpHeader>();
    auto opts = reinterpret_cast<uint32_t*>((&tcp_header) + 1);
    *opts = htonl(0x02040000 | (1460 & 0xFFFF));
    auto nop_ptr = reinterpret_cast<uint8_t*>(opts+1);
    nop_ptr[0] = 0x1; // NOP
    nop_ptr[1] = 0x3; // WS type
    nop_ptr[2] = 0x3; // opt length
    nop_ptr[3] = kWindowShift; // shift value
    entry->EnqueueSegment(tcp_header, std::move(new_buf), kTcpSyn | kTcpAck,
                          optlen);

    // Upcall application with new connection
    kassert(accept_fn);
    accept_fn(TcpPcb(entry));

    // Pass along the received data for processing (in case there is more data)
    if (info.tcplen > 1) {
      // In this case the data would need to be queued until we reach the
      // ESTABLISHED state
      kabort("UNIMPLEMENTED: Data with SYN packet\n");
    }

    auto f = [entry]() {
      // mark entry as valid to receive data
      entry->accepted = true;

      auto now = ebbrt::clock::Wall::Now();
      entry->Output(now);
      entry->SetTimer(now);
    };

    if (entry->cpu == Cpu::GetMine()) {
      f();
    } else {
      event_manager->SpawnRemote(std::move(f), entry->cpu);
    }
  }
}

// TCP Sequence computations
namespace {
// returns true if 'in' is in the interval [left, right] (inclusive)
bool TcpSeqBetween(uint32_t in, uint32_t left, uint32_t right) {
  return ((right - left) >= (in - left));
}

bool TcpSeqLT(uint32_t first, uint32_t second) {
  return ((int32_t)(first - second)) < 0;
}

bool TcpSeqGT(uint32_t first, uint32_t second) {
  return ((int32_t)(first - second)) > 0;
}

bool TcpSeqLEQ(uint32_t first, uint32_t second) {
  return ((int32_t)(first - second)) <= 0;
}

bool TcpSeqGEQ(uint32_t first, uint32_t second) {
  return ((int32_t)(first - second)) >= 0;
}
}  // namespace

// Send on a TCP connection
void ebbrt::NetworkManager::TcpEntry::Send(std::unique_ptr<IOBuf> buf) {
  // Prepend a header to the chain which will Ack any received data
  auto header_buf = MakeUniqueIOBuf(sizeof(TcpHeader) + sizeof(Ipv4Header) +
                                    sizeof(EthernetHeader));
  header_buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));
  header_buf->PrependChain(std::move(buf));
  auto dp = header_buf->GetMutDataPointer();
  auto& tcp_header = dp.Get<TcpHeader>();
  EnqueueSegment(tcp_header, std::move(header_buf), kTcpAck);
}

size_t ebbrt::NetworkManager::TcpEntry::SendWindowRemaining() {
#ifdef LARGE_WINDOW_HACK
  return (1 << 21) - static_cast<size_t>((snd_nxt - snd_una));
#else
  return snd_wnd - (snd_nxt - snd_una);
#endif
}

// Input on a TCP connection
void ebbrt::NetworkManager::TcpEntry::Input(const Ipv4Header& ih, TcpHeader& th,
                                            TcpInfo& info,
                                            std::unique_ptr<MutIOBuf> buf) {
  auto now = ebbrt::clock::Wall::Now();
  if (Receive(ih, th, info, std::move(buf), now)) {
    Output(now);
    SetTimer(now);
  }
}

// Returns true if the connection has not been aborted
bool ebbrt::NetworkManager::TcpEntry::Receive(
    const Ipv4Header& ih, TcpHeader& th, TcpInfo& info,
    std::unique_ptr<MutIOBuf> buf, ebbrt::clock::Wall::time_point now) {
  if (unlikely(state == kSynSent)) {
    auto flags = th.Flags();
    bool acceptable_ack = false;
    if (flags & kTcpAck) {
      // RFC 793 Page 66:
      // If the ACK bit is set:
      // If SEG.ACK =< ISS, or SEG.ACK > SND.NXT, send a reset (unless the RST
      // bit is set, if so drop the segment and return)
      if (TcpSeqLEQ(info.ackno, snd_una) || TcpSeqGT(info.ackno, snd_nxt)) {
        if (!(flags & kTcpRst)) {
          network_manager->TcpReset(false, info.ackno, 0, ih.dst, ih.src,
                                    info.dst_port, info.src_port);
          state = kClosed;
          handler->Abort();
          Purge();
          DisableTimers();
          Destroy();
          return false;
        } else {
          return true;
        }
      }
      acceptable_ack = true;
    }

    if (flags & kTcpRst) {
      // RFC 793 Page 67:
      // If the RST bit is set
      // If the ACK was acceptable then signal the user "error: connection
      // reset", drop the segment, enter CLOSED state, delete TCB, and return.
      // Otherwise (no ACK) drop the segment and return.
      if (acceptable_ack) {
        state = kClosed;
        handler->Abort();
        Purge();
        DisableTimers();
        Destroy();
        return false;
      } else {
        return true;
      }
    }

    if (flags & kTcpSyn) {
      // Either the ACK is acceptable or there is no ACK and no RST
      kassert((flags & kTcpAck && acceptable_ack) ||
              !(flags & (kTcpAck | kTcpRst)));
      rcv_nxt = info.seqno + 1;
      rcv_last_acked = info.seqno;
      if (acceptable_ack) {
        // Received a SYN-ACK
        snd_una = info.ackno;
        state = kEstablished;
        snd_wnd = ntohs(th.wnd) << kWindowShift;
        snd_wl1 = info.seqno;
        snd_wl2 = info.ackno;

        ClearAckedSegments(info);

        if (info.tcplen > 1) {
          kabort("UNIMPLEMENTED: Data with SYN packet\n");
        }

        handler->Connected();
      } else {
        kabort("UNIMPLEMENTED: Simultaneous Open\n");
      }
    }
  } else {
    auto flags = th.Flags();

    // XXX: I believe the spec says ACK processing must happen after validating
    // the sequence, but that can cause poor performance in some degenerative
    // cases so we do it here
    if (likely(TcpSeqGEQ(info.seqno, rcv_nxt) && flags & kTcpAck &&
               !(flags & kTcpSyn) && !(flags & kTcpRst))) {
      // Common case: Not RST and not SYN
      if (unlikely(state == kSynReceived)) {
        // RFC 793 Page 72:
        // If SND.UNA =< SEG.ACK =< SND.NXT then enter ESTABLISHED state and
        // continue processing.
        // If the segment acknowledgment is not acceptable, form a reset
        // segment,
        //    <SEQ=SEG.ACK><CTL=RST>
        // and send it.
        if (!TcpSeqBetween(info.ackno, snd_una + 1, snd_nxt)) {
          network_manager->TcpReset(false, info.ackno, 0, ih.dst, ih.src,
                                    info.dst_port, info.src_port);
          return true;
        }

        state = kEstablished;
        snd_wnd = ntohs(th.wnd) << kWindowShift;
        snd_wl1 = info.seqno;
        snd_wl2 = info.ackno;
        // Fall through
      }

      if (likely(state <= kClosing)) {
        // Common case: In a connected state
        if (TcpSeqBetween(info.ackno, snd_una, snd_nxt)) {
          // SND.UNA =< SEG.ACK =< SND.NEXT
          snd_una = info.ackno;

          if (TcpSeqBetween(info.ackno, snd_una + 1, snd_nxt) ||
              TcpSeqLT(snd_wl1, info.seqno) ||
              (snd_wl1 == info.seqno && TcpSeqLEQ(snd_wl2, info.ackno))) {
            // RFC 793 page 72:
            // "If SND.UNA < SEG.ACK =< SND.NXT, the send window should be
            // updated.  If (SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and
            // SND.WL2 =< SEG.ACK)), set SND.WND <- SEG.WND, set SND.WL1 <-
            // SEG.SEQ, and set SND.WL2 <- SEG.ACK."
            // ... "The check here prevents using old segments to update the
            // window"
            snd_wnd = ntohs(th.wnd) << kWindowShift;
            snd_wl1 = info.seqno;
            snd_wl2 = info.ackno;
          }

          ClearAckedSegments(info);

          if (window_notify) {
            // Upcall user that the send window has increased
            handler->SendWindowIncrease();
          }
        } else if (unlikely(TcpSeqGT(info.ackno, snd_nxt))) {
          // RFC 793: Page 72
          // "If the ACK acks something not yet sent (SEG.ACK > SND.NXT) then
          // send an ACK, drop the segment, and return."
          SendEmptyAck();
          return true;
        } else {
          // RFC 793: Page 72 (fixed by RFC 1122 page 94)
          // "If the ACK is a duplicate (SEG.ACK <= SND.UNA), it can be
          // ignored."
        }

        // Additional ACK processing for some closing states
        if (unlikely(state > kEstablished)) {
          if (state == kFinWait1) {
            // If our FIN is now acknowledged we can enter FIN-WAIT-2 and
            // continue
            if (info.ackno == snd_nxt)
              state = kFinWait2;
          } else if (state == kClosing) {
            // If our FIN is now acknowledged we can enter TIME-WAIT
            if (info.ackno == snd_nxt) {
              Purge();
              DisableTimers();

              time_wait = now + std::chrono::seconds(60);
              state = kTimeWait;
            }
          }
        }
      } else if (state == kLastAck) {
        if (info.ackno == snd_nxt) {
          // Our Fin was acked, clean up everything
          state = kClosed;
          Purge();
          DisableTimers();
          Destroy();
          return false;
        }
      }
    }

    // First check sequence number
    if (likely((rcv_wnd > 0 &&
                TcpSeqBetween(info.seqno, rcv_nxt, rcv_nxt + rcv_wnd))) ||
        (info.tcplen == 0 && info.seqno == rcv_nxt)) {
      // 1) Segment is within the sequence of the receive window
      //    RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
      // 2) this sequence has zero length but is in sequence (even if our
      // receive window is closed)

      if (TcpSeqGT(rcv_nxt, info.seqno)) {
        // Trim the front
        kprintf(">> rcv_nxt > info.seqno \n");
        buf->Advance(rcv_nxt - info.seqno);
        info.tcplen -= rcv_nxt - info.seqno;
      }

      // Second check the RST bit
      if (unlikely(flags & kTcpRst)) {
        state = kClosed;
        if (state == kSynReceived) {
          // RFC 793 Page 70:
          // "If this connection was initiated with a passive OPEN (i.e., came
          // from the LISTEN state), then return this connection to LISTEN state
          // and return.  The user need not be informed.  If this connection was
          // initiated with an active OPEN (i.e., came from SYN-SENT state) then
          // the connection was refused, signal the user "connection refused".
          // In either case, all segments on the retransmission queue should be
          // removed.  And in the active OPEN case, enter the CLOSED state and
          // delete the TCB, and return."

          handler->Abort();
        } else if (state >= kEstablished && state <= kCloseWait) {
          // If the RST bit is set then, any outstanding RECEIVEs and SEND
          // should receive "reset" responses.  All segment queues should be
          // flushed.  Users should also receive an unsolicited general
          // "connection reset" signal.  Enter the CLOSED state, delete the TCB,
          // and return.
          handler->Abort();
        } else {
          // CLOSING, LAST-ACK, or TIME-WAIT state
          // The user has called Close() and we have already received a Fin from
          // the remote side so we do not need to notify the user
          // RFC 793 Page 70:
          // If the RST bit is set then, enter the CLOSED state, delete the TCB,
          // and return.
        }
        Purge();
        DisableTimers();
        Destroy();
        return false;
      } else if (unlikely(flags & kTcpSyn)) {
        // RFC 793 Page 71:
        // "If the SYN is in the window it is an error, send a reset, any
        // outstanding RECEIVEs and SEND should receive "reset" responses, all
        // segment queues should be flushed, the user should also receive an
        // unsolicited general "connection reset" signal, enter the CLOSED
        // state, delete the TCB, and return."

        // XXX: Not sure if this seqno is right, but it *should* make the RST in
        // sequence and therefore accepted by the remote side. Also not sure if
        // the ACK is right.
        network_manager->TcpReset(true, info.ackno, info.seqno + info.tcplen,
                                  ih.dst, ih.src, info.dst_port, info.src_port);
        state = kClosed;
        handler->Abort();
        Purge();
        DisableTimers();
        Destroy();
        return false;
      } else if (unlikely(!(flags & kTcpAck))) {
        // If a segment does not have a RST SYN or ACK then it gets dropped
        return true;
      }

      if (flags & kTcpUrg) {
        kabort("UNIMPLEMENTED: URG bit set\n");
      }

      // Process payload
      auto buf_len = buf->ComputeChainDataLength();
      auto hdr_len = th.HdrLen();
      auto payload_len = buf_len - hdr_len;
      if (payload_len > 0) {
        if (likely(state == kEstablished)) {

          // Stash segments that are not in-sequence
          // TODO: save and restore flags of stashed segments
          if (TcpSeqGT(info.seqno, rcv_nxt)) {
            // RFC 793 Page 69:
            // "Segments with higher begining sequence numbers may be held for
            // later processing."
            buf->Advance(hdr_len);
            if (stashed_segments.count(info.seqno) == 0) {
              stashed_segments.emplace(info.seqno, std::move(buf));
            }
            SendEmptyAck();
            return true;
          }
          // Append stashed in-sequence segments
          if (!stashed_segments.empty()) {
            auto it = stashed_segments.begin();
            while (it != stashed_segments.end()) {
              if (it->first == rcv_nxt + payload_len) {
                payload_len += (it->second)->ComputeChainDataLength();
                buf->PrependChain(std::move(it->second));
                it = stashed_segments.erase(it);
              } else {
                ++it;
              }
            }
          }
          // From here all received data should be in-sequence
          if (unlikely(payload_len > rcv_wnd)) {
            if (flags & kTcpFin) {
              // remove FIN flag since it doesn't fit in the window
              auto flags = th.Flags() & ~kTcpFin;
              const_cast<TcpHeader&>(th).SetFlags(flags);
              info.tcplen--;
            }
            // Received more data than our receive window can hold, trim the end
            buf->TrimEnd(payload_len - rcv_wnd);
            payload_len = rcv_wnd;
          }

          rcv_nxt += payload_len;
          if (unlikely(close_window)) {
            rcv_wnd -= payload_len;
          }

          buf->Advance(hdr_len);
          handler->Receive(std::move(buf));
        } else if (state == kFinWait1 || state == kFinWait2) {
          // The application has already called Close(), but we have not
          // received a FIN yet. Normally, received data in this case should
          // still be sent to the application (because the close really only
          // closes our send side). However we implement the following from RFC
          // 1122 4.2.2.13 Page 88 Closing a connection:
          // "A host MAY implement a "half-duplex" TCP close sequence, so that
          // an application that has called CLOSE cannot continue to read data
          // from the connection.  If such a host issues a CLOSE call while
          // received data is still pending in TCP, or if new data is received
          // after CLOSE is called, its TCP SHOULD send a RST to show that data
          // was lost.
          network_manager->TcpReset(true, info.ackno, info.seqno + info.tcplen,
                                    ih.dst, ih.src, info.dst_port,
                                    info.src_port);
          state = kClosed;
          Purge();
          DisableTimers();
          Destroy();
          return false;
        }
      }

      if (flags & kTcpFin) {
        if (unlikely(state == kSynSent)) {
          // Do not process the FIN if the state is CLOSED, LISTEN or SYN-SENT
          // since the SEG.SEQ cannot be validated; drop the segment and return.
          return true;
        }

        rcv_nxt = info.seqno + info.tcplen;
        if (state == kEstablished || state == kSynReceived) {
          state = kCloseWait;
          handler->Close();
        } else if (state == kFinWait1) {
          // We can only be in FinWait1 if our FIN wasn't ACKed so this is a
          // simultaneous close, we must transition to the CLOSING state
          state = kClosing;
        } else if (state == kFinWait2) {
          // Transition to TIME-WAIT
          DisableTimers();
          time_wait = now + std::chrono::seconds(60);
          state = kTimeWait;
        } else if (state == kTimeWait) {
          // Received another FIN, restart the timeout
          DisableTimers();
          time_wait = now + std::chrono::seconds(60);
        }
      }
    } else if (pending_segments.empty()) {
      // We reach here if we received an out of sequence segment and we don't
      // already have pending segments to send (due to this segment acking data
      // and opening our window, letting us send more)

      // RFC 793 Page 69:
      // "If an incoming segment is not acceptable, an acknowledgment should be
      // sent in reply (unless the RST bit is set, if so drop the segment and
      // return)
      // After sending the acknowledgment, drop the unacceptable segment and
      // return."
      if (!(th.Flags() & kTcpRst)) {
        SendEmptyAck();
      }
    }
  }
  return true;
}

void ebbrt::NetworkManager::TcpEntry::ClearAckedSegments(const TcpInfo& info) {
  // Function to clear acked segments from a queue
  auto clear_acked_segments =
      [&info](boost::container::list<TcpSegment>& queue) {
        auto it = queue.begin();
        while (it != queue.end()) {
          if (TcpSeqGT(ntohl(it->th.seqno) + it->SeqLen(), info.ackno))
            break;
          auto prev_it = it++;
          queue.erase(prev_it);
        }
      };

  // Remove all unacked segments that have been completely acked by
  // this ACK
  clear_acked_segments(unacked_segments);
  // Its also possible to find pending segments which have been ACKed
  // because we may have hit a retransmit timer which would cause them
  // to be moved back to the pending_segments queue. Remove them
  clear_acked_segments(pending_segments);

  if (unacked_segments.empty()) {
    // The only timer that could be active here is our retransmit
    // timer so we are safe to disable all timers
    DisableTimers();
  }
}

// Fill out a header and enqueue the segment to be sent
void ebbrt::NetworkManager::TcpEntry::EnqueueSegment(
    TcpHeader& th, std::unique_ptr<MutIOBuf> buf, uint16_t flags,
    uint16_t optlen) {
  auto packet_len = buf->ComputeChainDataLength();
  auto tcp_len = packet_len - sizeof(TcpHeader) - optlen +
                 ((flags & (kTcpSyn | kTcpFin)) ? 1 : 0);
  th.src_port = htons(std::get<2>(key));
  th.dst_port = htons(std::get<1>(key));
  th.seqno = htonl(snd_nxt);
  th.SetHdrLenFlags(sizeof(TcpHeader) + optlen, flags);
  // ackno, wnd, and checksum are set in Output()
  th.urgp = 0;

  pending_segments.emplace_back(std::move(buf), th, tcp_len);

  snd_nxt += tcp_len;
}

// Attempt to send any outstanding tcp segments
size_t
ebbrt::NetworkManager::TcpEntry::Output(ebbrt::clock::Wall::time_point now) {
  auto it = pending_segments.begin();

  // try to send as many pending segments as will fit in the window
  size_t sent = 0;
  for (; it != pending_segments.end() &&
         TcpSeqLEQ(ntohl(it->th.seqno) + it->tcp_len,
                   snd_nxt + SendWindowRemaining());
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
      if (snd_wnd > 0) {
        // Shrink the front most segment so it will fit in the window
        kabort("UNIMPLEMENTED: Shrink pending segment to be sent\n");
      } else {
        // Set a persist timer to periodically probe for window size changes
        kabort("UNIMPLEMENTED: Window size probe needed\n");
      }
    }

    // In the case that we don't have any data to send out but we have received
    // data since our last ACK, we will send an empty ACK
    if (TcpSeqLT(rcv_last_acked, rcv_nxt)) {
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
  auto buf = MakeUniqueIOBuf(sizeof(TcpHeader) + sizeof(Ipv4Header) +
                             sizeof(EthernetHeader));
  buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));
  auto dp = buf->GetMutDataPointer();
  auto& th = dp.Get<TcpHeader>();
  th.src_port = htons(std::get<2>(key));
  th.dst_port = htons(std::get<1>(key));
  th.seqno = htonl(snd_nxt);
  th.SetHdrLenFlags(sizeof(TcpHeader), kTcpAck);
  th.urgp = 0;
  rcv_last_acked = rcv_nxt;
  th.ackno = htonl(rcv_nxt);
  th.wnd = htons(TcpWindow16(rcv_wnd));
  th.checksum = OffloadPseudoCsum(*buf, kIpProtoTCP, address, std::get<0>(key));

  PacketInfo pinfo;
  pinfo.flags |= PacketInfo::kNeedsCsum;
  pinfo.csum_start = 0;
  pinfo.csum_offset = 16;  // checksum is 16 bytes into the TCP header

  network_manager->SendIp(std::move(buf), address, std::get<0>(key),
                          kIpProtoTCP, pinfo);
}

// When Close() is called we will send a FIN and wait for all outstanding
// segments to be acked before deleting the entry
void ebbrt::NetworkManager::TcpEntry::Close() {
  switch (state) {
  case kClosed:
    break;
  case kCloseWait:
    // Passive close, Send FIN and wait for the last ack then terminate
    SendFin();
    state = kLastAck;
    break;
  case kSynReceived:
  case kEstablished:
    // Active close, Send FIN and wait for FIN and ACK
    SendFin();
    state = kFinWait1;
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
      seg.th.SetFlags(flags | kTcpFin);
      ++seg.tcp_len;
      ++snd_nxt;
      return;
    }
  }

  // Otherwise create an empty segment with a Fin
  auto buf = MakeUniqueIOBuf(sizeof(TcpHeader) + sizeof(Ipv4Header) +
                             sizeof(EthernetHeader));
  buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));
  auto dp = buf->GetMutDataPointer();
  auto& tcp_header = dp.Get<TcpHeader>();
  EnqueueSegment(tcp_header, std::move(buf), kTcpFin | kTcpAck);
}

// Actually send a segment via IP
void ebbrt::NetworkManager::TcpEntry::SendSegment(TcpSegment& segment) {
  rcv_last_acked = rcv_nxt;
  segment.th.ackno = htonl(rcv_nxt);
  segment.th.wnd = htons(TcpWindow16(rcv_wnd));
  segment.th.checksum = 0;
  // XXX: check if checksum offloading is supported
  segment.th.checksum =
      OffloadPseudoCsum(*(segment.buf), kIpProtoTCP, address, std::get<0>(key));
  PacketInfo pinfo;
  pinfo.flags |= PacketInfo::kNeedsCsum;
  pinfo.csum_start = 0;
  pinfo.csum_offset = 16;  // checksum is 16 bytes into the TCP header

  // XXX: Actually store the MSS instead of making this assumption
  size_t mss = 1460;
  if (segment.tcp_len > mss) {
    pinfo.gso_type = PacketInfo::kGsoTcpv4;
    pinfo.hdr_len = segment.th.HdrLen();
    pinfo.gso_size = mss;
  }

  network_manager->SendIp(CreateRefChain(*(segment.buf)), address,
                          std::get<0>(key), kIpProtoTCP, std::move(pinfo));
}

// Send a reset packet
void ebbrt::NetworkManager::TcpReset(bool ack, uint32_t seqno, uint32_t ackno,
                                     const Ipv4Address& local_ip,
                                     const Ipv4Address& remote_ip,
                                     uint16_t local_port,
                                     uint16_t remote_port) {
  auto buf = MakeUniqueIOBuf(sizeof(TcpHeader) + sizeof(Ipv4Header) +
                             sizeof(EthernetHeader));

  buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));

  auto dp = buf->GetMutDataPointer();
  auto& tcp_header = dp.Get<TcpHeader>();

  tcp_header.src_port = htons(local_port);
  tcp_header.dst_port = htons(remote_port);
  tcp_header.seqno = htonl(seqno);
  tcp_header.ackno = htonl(ackno);
  tcp_header.SetHdrLenFlags(sizeof(TcpHeader), kTcpRst | (ack ? kTcpAck : 0));
  tcp_header.wnd = htons(TcpWindow16(kTcpWnd));
  tcp_header.urgp = 0;
  tcp_header.checksum =
      OffloadPseudoCsum(*buf, kIpProtoTCP, local_ip, remote_ip);

  PacketInfo pinfo;
  pinfo.flags |= PacketInfo::kNeedsCsum;
  pinfo.csum_start = 0;  // 14 byte eth header + 20 byte ip header
  pinfo.csum_offset = 16;  // checksum is 16 bytes into the TCP header

  SendIp(std::move(buf), local_ip, remote_ip, kIpProtoTCP, pinfo);
}
