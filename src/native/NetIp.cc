//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Net.h"

ebbrt::Ipv4Address ebbrt::NetworkManager::IpAddress() {
  if (interface_)
    return interface_->Address()->address;
  else
    std::runtime_error("No interface to get IP address");
  return 0;
}

// Is addr a broadcast address on THIS interface?
bool ebbrt::NetworkManager::Interface::ItfAddress::isBroadcast(
    const Ipv4Address& addr) const {
  if (addr.isBroadcast())
    return true;

  if (addr == address)
    return false;

  // if on the same subnet and host bits are all set
  if (isLocalNetwork(addr) &&
      addr.Mask(~netmask) == Ipv4Address::Broadcast().Mask(~netmask))
    return true;

  return false;
}

// Is addr on this subnet?
bool ebbrt::NetworkManager::Interface::ItfAddress::isLocalNetwork(
    const Ipv4Address& addr) const {
  return addr.Mask(netmask) == address.Mask(netmask);
}

// Receive an Ipv4 packet
void ebbrt::NetworkManager::Interface::ReceiveIp(EthernetHeader& eth_header,
                                                 std::unique_ptr<MutIOBuf> buf,
                                                 uint64_t rxflag) {
  auto packet_len = buf->ComputeChainDataLength();

  if (unlikely(packet_len < sizeof(Ipv4Header)))
    return;

  auto dp = buf->GetMutDataPointer();
  auto& ip_header = dp.Get<Ipv4Header>();

  if (unlikely(ip_header.Version() != 4))
    return;

  auto hlen = ip_header.HeaderLength();
  if (unlikely(hlen < sizeof(Ipv4Header)))
    return;

  auto tot_len = ip_header.TotalLength();
  if (unlikely(packet_len < tot_len))
    return;

  buf->TrimEnd(packet_len - tot_len);

#ifdef __EBBRT_ENABLE_BAREMETAL_NIC__
  // baremetal checksum offload
  if (unlikely((rxflag & RXFLAG_IPCS) == 0)) {
    ebbrt::kprintf("%s RXFLAG_IPCS failed\n", __FUNCTION__);
    return;
  }

  if (unlikely((rxflag & RXFLAG_IPCS_VALID) == 0)) {
    ebbrt::kprintf("%s RXFLAG_IPCS_VALID failed\n", __FUNCTION__);
    return;
  }
#else
  if (unlikely(ip_header.ComputeChecksum() != 0))
    return;
#endif

  auto addr = Address();
  // Unless the protocol is UDP or we have an address on this interface and the
  // packet is directed to us or broadcast on our subnet, then drop. We allow
  // UDP through for DHCP to work before we have an address.
  if (unlikely(ip_header.proto != kIpProtoUDP &&
               (!addr || (!addr->isBroadcast(ip_header.dst) &&
                          addr->address != ip_header.dst))))
    return;

  // Drop unacceptable sources
  if (unlikely(ip_header.src.isBroadcast() || ip_header.src.isMulticast()))
    return;

  // We do not support fragmentation
  if (unlikely(ip_header.Fragmented()))
    return;

  buf->Advance(hlen);

  switch (ip_header.proto) {
  case kIpProtoICMP: {
    ReceiveIcmp(eth_header, ip_header, std::move(buf));
    break;
  }
  case kIpProtoUDP: {
    ReceiveUdp(ip_header, std::move(buf), rxflag);
    break;
  }
  case kIpProtoTCP: {
    ReceiveTcp(ip_header, std::move(buf), rxflag);
    break;
  }
  }
}

void ebbrt::NetworkManager::SendIp(std::unique_ptr<MutIOBuf> buf,
                                   Ipv4Address src, Ipv4Address dst,
                                   uint8_t proto, PacketInfo pinfo) {
  // find the interface to route to, then send it on that interface
  auto itf = IpRoute(dst);
  if (likely(itf != nullptr))
    itf->SendIp(std::move(buf), src, dst, proto, std::move(pinfo));
}

// Send IP packet out on this interface
void ebbrt::NetworkManager::Interface::SendIp(std::unique_ptr<MutIOBuf> buf,
                                              Ipv4Address src, Ipv4Address dst,
                                              uint8_t proto, PacketInfo pinfo) {
  buf->Retreat(sizeof(Ipv4Header));
  auto dp = buf->GetMutDataPointer();
  auto& ih = dp.Get<Ipv4Header>();
  ih.version_ihl = 4 << 4 | 5;
  ih.dscp_ecn = 0;
  ih.length = ntohs(buf->ComputeChainDataLength());
  ih.id = 0;
  ih.flags_fragoff = 0;
  ih.ttl = kIpDefaultTtl;
  ih.proto = proto;
  ih.chksum = 0;
  ih.src = src;
  ih.dst = dst;

#ifdef __EBBRT_ENABLE_BAREMETAL_NIC__
  // baremetal ip checksum offload
  pinfo.flags |= PacketInfo::kNeedsIpCsum;
#else
  ih.chksum = ih.ComputeChecksum();
  kassert(ih.ComputeChecksum() == 0);
#endif

  pinfo.csum_start += sizeof(Ipv4Header);
  pinfo.hdr_len += sizeof(Ipv4Header);

  EthArpSend(kEthTypeIp, ih, std::move(buf), pinfo);
}

// This finds the interface to send on, given a destination
ebbrt::NetworkManager::Interface*
ebbrt::NetworkManager::IpRoute(Ipv4Address dest) {
  // Silly implementation for now, if we have an interface, then return that,
  // otherwise nothing
  if (!interface_)
    return nullptr;

  return &*interface_;
}
