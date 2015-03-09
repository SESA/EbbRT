//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Net.h>

#include <ebbrt/NetChecksum.h>
#include <ebbrt/NetUdp.h>
#include <ebbrt/UniqueIOBuf.h>

// Close a listening connection. Note that Receive could still be called until
// the future is fulfilled
ebbrt::Future<void> ebbrt::NetworkManager::UdpPcb::Close() {
  if (entry_->port) {
    network_manager->udp_port_allocator_->Free(entry_->port);
    std::lock_guard<ebbrt::SpinLock> guard(network_manager->udp_write_lock_);
    network_manager->udp_pcbs_.erase(*entry_);
    entry_->port = 0;
  }
  // wait for an RCU grace period to ensure that Receive can no longer be called
  return CallRcu([]() {});
}

// destroy entry
void ebbrt::NetworkManager::UdpPcb::UdpEntryDeleter::operator()(UdpEntry* e) {
  kassert(e->port == 0);
  delete e;
}

// Bind and listen on a port (0 to allocate one). The return value is the bound
// port
uint16_t ebbrt::NetworkManager::UdpPcb::Bind(uint16_t port) {
  if (!port) {
    auto ret = network_manager->udp_port_allocator_->Allocate();
    if (!ret)
      throw std::runtime_error("Failed to allocate ephemeral port");

    port = *ret;
  } else if (port >= 49162 &&
             !network_manager->udp_port_allocator_->Reserve(port)) {
    throw std::runtime_error("Failed to reserve specified port");
  }

  entry_->port = port;
  {
    std::lock_guard<ebbrt::SpinLock> guard(network_manager->udp_write_lock_);
    network_manager->udp_pcbs_.insert(*entry_);
  }
  return port;
}

// Setup the receive function for a UDP pcb
void ebbrt::NetworkManager::UdpPcb::Receive(MovableFunction<
    void(Ipv4Address, uint16_t, std::unique_ptr<MutIOBuf>)> func) {
  entry_->func = std::move(func);
}

// Receive UDP packet on an interface
void
ebbrt::NetworkManager::Interface::ReceiveUdp(Ipv4Header& ip_header,
                                             std::unique_ptr<MutIOBuf> buf) {
  auto packet_len = buf->ComputeChainDataLength();

  // Ensure we have a header
  if (unlikely(packet_len < sizeof(UdpHeader)))
    return;

  auto dp = buf->GetDataPointer();
  const auto& udp_header = dp.Get<UdpHeader>();

  // ensure packet is the full udp packet
  if (unlikely(packet_len < ntohs(udp_header.length)))
    return;

  // trim any excess off the packet
  buf->TrimEnd(packet_len - ntohs(udp_header.length));

  if (udp_header.checksum &&
      IpPseudoCsum(*buf, ip_header.proto, ip_header.src, ip_header.dst))
    return;

  auto entry = network_manager->udp_pcbs_.find(ntohs(udp_header.dst_port));

  if (!entry)
    return;

  buf->Advance(sizeof(UdpHeader));

  entry->func(ip_header.src, ntohs(udp_header.src_port), std::move(buf));
}

// Send a UDP packet. The UdpPcb must be bound.
void ebbrt::NetworkManager::UdpPcb::SendTo(Ipv4Address addr, uint16_t port,
                                           std::unique_ptr<IOBuf> buf) {
  auto itf = network_manager->IpRoute(addr);

  if (!itf)
    return;

  itf->SendUdp(*this, addr, port, std::move(buf));
}

// Send a udp packet on an interface
void ebbrt::NetworkManager::Interface::SendUdp(UdpPcb& pcb, Ipv4Address addr,
                                               uint16_t port,
                                               std::unique_ptr<IOBuf> buf) {
  // Get source address
  auto itf_addr = Address();
  Ipv4Address src_addr;
  if (!itf_addr) {
    src_addr = Ipv4Address::Any();
  } else {
    src_addr = itf_addr->address;
  }

  // Get source port
  auto src_port = pcb.entry_->port;
  if (!src_port)
    throw std::runtime_error("Send on unbound pcb");

  // Construct header
  auto header_buf = MakeUniqueIOBuf(sizeof(UdpHeader) + sizeof(Ipv4Header) +
                                    sizeof(EthernetHeader));
  header_buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));
  auto dp = header_buf->GetMutDataPointer();
  auto& udp_header = dp.Get<UdpHeader>();
  udp_header.src_port = htons(src_port);
  udp_header.dst_port = htons(port);
  udp_header.length = htons(buf->ComputeChainDataLength() + sizeof(UdpHeader));
  udp_header.checksum = 0;

  // Append data
  header_buf->AppendChain(std::move(buf));

  // XXX: check if checksum offloading is supported
  PacketInfo pinfo;
  pinfo.flags |= PacketInfo::kNeedsCsum;
  pinfo.csum_start = 34;
  pinfo.csum_offset = 6;

  SendIp(std::move(header_buf), src_addr, addr, kIpProtoUDP, std::move(pinfo));
}
