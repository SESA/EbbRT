//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Net.h>

#include <cinttypes>

#include <ebbrt/NetChecksum.h>
#include <ebbrt/NetIcmp.h>
#include <ebbrt/NetUdp.h>
#include <ebbrt/Random.h>
#include <ebbrt/Timer.h>
#include <ebbrt/UniqueIOBuf.h>

void ebbrt::NetworkManager::Init() {}

ebbrt::NetworkManager::Interface&
ebbrt::NetworkManager::NewInterface(EthernetDevice& ether_dev) {
  interface_.reset(new Interface(ether_dev));
  return *interface_;
}

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

bool ebbrt::NetworkManager::Interface::ItfAddress::isLocalNetwork(
    const Ipv4Address& addr) const {
  return addr.Mask(netmask) == address.Mask(netmask);
}

void ebbrt::NetworkManager::Interface::Receive(std::unique_ptr<MutIOBuf> buf) {
  auto packet_len = buf->ComputeChainDataLength();

  // Drop packets that are too small
  if (packet_len <= sizeof(EthernetHeader))
    return;

  auto dp = buf->GetMutDataPointer();
  auto& eth_header = dp.Get<EthernetHeader>();

  buf->Advance(sizeof(EthernetHeader));

  switch (ntohs(eth_header.type)) {
  case kEthTypeIp: {
    ReceiveIp(eth_header, std::move(buf));
    break;
  }
  case kEthTypeArp: {
    ReceiveArp(eth_header, std::move(buf));
    break;
  }
  }
}

ebbrt::Future<void> ebbrt::NetworkManager::Interface::StartDhcp() {
  dhcp_pcb_.udp_pcb.Bind(kDhcpClientPort);
  dhcp_pcb_.udp_pcb.Receive([this](Ipv4Address from_addr, uint16_t from_port,
                                   std::unique_ptr<MutIOBuf> buf) {
    ReceiveDhcp(from_addr, from_port, std::move(buf));
  });
  std::lock_guard<std::mutex> guard(dhcp_pcb_.lock);
  DhcpDiscover();
  return dhcp_pcb_.complete.GetFuture();
}

void ebbrt::NetworkManager::Interface::DhcpOption(DhcpMessage& message,
                                                  uint8_t option_type,
                                                  uint8_t option_len) {
  auto& len = dhcp_pcb_.options_len;
  kassert((len + 2) <= kDhcpOptionsLen);
  message.options[len++] = option_type;
  message.options[len++] = option_len;
}

void ebbrt::NetworkManager::Interface::DhcpOptionByte(DhcpMessage& message,
                                                      uint8_t value) {
  auto& len = dhcp_pcb_.options_len;
  kassert((len + 1) <= kDhcpOptionsLen);
  message.options[len++] = value;
}

// void ebbrt::NetworkManager::Interface::DhcpOptionShort(DhcpMessage& message,
//                                                        uint8_t value) {}

void ebbrt::NetworkManager::Interface::DhcpOptionLong(DhcpMessage& message,
                                                      uint32_t value) {
  auto& len = dhcp_pcb_.options_len;
  kassert((len + 4) <= kDhcpOptionsLen);
  *reinterpret_cast<uint32_t*>(message.options.data() + len) = value;
  len += 4;
}

void ebbrt::NetworkManager::Interface::DhcpOptionTrailer(DhcpMessage& message) {
  auto& len = dhcp_pcb_.options_len;
  kassert((len + 1) <= kDhcpOptionsLen);
  message.options[len++] = kDhcpOptionEnd;
}

boost::optional<uint8_t>
ebbrt::NetworkManager::Interface::DhcpGetOptionByte(const DhcpMessage& message,
                                                    uint8_t option_type) {
  auto option_it = message.options.begin();
  while (option_it != message.options.end()) {
    auto option_code = option_it[0];

    if (option_code == option_type)
      return boost::optional<uint8_t>(option_it[2]);  // skip code and length

    if (option_code == kDhcpOptionEnd)
      return boost::optional<uint8_t>();

    auto option_length = option_it[1];
    std::advance(option_it, option_length + 2);
  }

  return boost::optional<uint8_t>();
}

boost::optional<uint32_t>
ebbrt::NetworkManager::Interface::DhcpGetOptionLong(const DhcpMessage& message,
                                                    uint8_t option_type) {
  auto option_it = message.options.begin();
  while (option_it != message.options.end()) {
    auto option_code = option_it[0];

    if (option_code == option_type)
      return boost::optional<uint32_t>(
          *reinterpret_cast<const uint32_t*>(&option_it[2]));

    if (option_code == kDhcpOptionEnd)
      return boost::optional<uint32_t>();

    auto option_length = option_it[1];
    std::advance(option_it, option_length + 2);
  }

  return boost::optional<uint32_t>();
}

void ebbrt::NetworkManager::Interface::DhcpSetState(DhcpPcb::State state) {
  if (state != dhcp_pcb_.state) {
    dhcp_pcb_.state = state;
    dhcp_pcb_.tries = 0;
  }
}

void ebbrt::NetworkManager::Interface::DhcpCreateMessage(DhcpMessage& message) {
  if (dhcp_pcb_.tries == 0)
    dhcp_pcb_.xid = random::Get();

  message.op = kDhcpOpRequest;
  message.htype = kDhcpHTypeEth;
  message.hlen = kDhcpHLenEth;
  message.xid = htonl(dhcp_pcb_.xid);
  const auto& mac_addr = MacAddress();
  std::copy(mac_addr.begin(), mac_addr.end(), message.chaddr.begin());
  message.cookie = htonl(kDhcpMagicCookie);
  dhcp_pcb_.options_len = 0;
}

void ebbrt::NetworkManager::Interface::DhcpDiscover() {
  DhcpSetState(DhcpPcb::State::kSelecting);
  auto buf = IOBuf::Create<MutUniqueIOBuf>(sizeof(DhcpMessage));
  auto dp = buf->GetMutDataPointer();
  auto& dhcp_message = dp.Get<DhcpMessage>();

  DhcpCreateMessage(dhcp_message);

  DhcpOption(dhcp_message, kDhcpOptionMessageType, 1);
  DhcpOptionByte(dhcp_message, kDhcpOptionMessageTypeDiscover);

  DhcpOption(dhcp_message, kDhcpOptionParameterRequestList, 3);
  DhcpOptionByte(dhcp_message, kDhcpOptionSubnetMask);
  DhcpOptionByte(dhcp_message, kDhcpOptionRouter);
  DhcpOptionByte(dhcp_message, kDhcpOptionBroadcast);

  DhcpOptionTrailer(dhcp_message);

  dhcp_pcb_.udp_pcb.SendTo(Ipv4Address::Broadcast(), kDhcpServerPort,
                           std::move(buf));

  dhcp_pcb_.tries++;
  auto timeout =
      std::chrono::seconds(dhcp_pcb_.tries < 6 ? 1 << dhcp_pcb_.tries : 60);
  timer->Start(dhcp_pcb_, timeout, /* repeat = */ false);
}

void ebbrt::NetworkManager::Interface::DhcpHandleOffer(
    const DhcpMessage& dhcp_message) {
  timer->Stop(dhcp_pcb_);
  auto server_id_opt = DhcpGetOptionLong(dhcp_message, kDhcpOptionServerId);
  if (!server_id_opt)
    return;

  const auto& offered_ip = dhcp_message.yiaddr;

  DhcpSetState(DhcpPcb::State::kRequesting);
  auto buf = IOBuf::Create<MutUniqueIOBuf>(sizeof(DhcpMessage));
  auto dp = buf->GetMutDataPointer();
  auto& new_dhcp_message = dp.Get<DhcpMessage>();
  DhcpCreateMessage(new_dhcp_message);

  DhcpOption(new_dhcp_message, kDhcpOptionMessageType, 1);
  DhcpOptionByte(new_dhcp_message, kDhcpOptionMessageTypeRequest);

  DhcpOption(new_dhcp_message, kDhcpOptionRequestIp, 4);
  DhcpOptionLong(new_dhcp_message, offered_ip.toU32());

  DhcpOption(new_dhcp_message, kDhcpOptionServerId, 4);
  DhcpOptionLong(new_dhcp_message, *server_id_opt);

  DhcpOption(new_dhcp_message, kDhcpOptionParameterRequestList, 3);
  DhcpOptionByte(new_dhcp_message, kDhcpOptionSubnetMask);
  DhcpOptionByte(new_dhcp_message, kDhcpOptionRouter);
  DhcpOptionByte(new_dhcp_message, kDhcpOptionBroadcast);

  DhcpOptionTrailer(new_dhcp_message);

  dhcp_pcb_.udp_pcb.SendTo(Ipv4Address::Broadcast(), kDhcpServerPort,
                           std::move(buf));

  dhcp_pcb_.tries++;
  auto timeout =
      std::chrono::seconds(dhcp_pcb_.tries < 6 ? 1 << dhcp_pcb_.tries : 60);
  timer->Start(dhcp_pcb_, timeout,
               /* repeat = */ false);
}

void ebbrt::NetworkManager::Interface::DhcpPcb::Fire() {
  std::lock_guard<std::mutex> guard(lock);
  switch (state) {
  case DhcpPcb::State::kRequesting: {
    kabort("UNIMPLEMENTED: Dhcp request timeout\n");
    break;
  }
  case DhcpPcb::State::kSelecting: {
    auto itf =
        boost::intrusive::get_parent_from_member(this, &Interface::dhcp_pcb_);
    itf->DhcpDiscover();
    break;
  }
  case DhcpPcb::State::kBound: {
    auto now = ebbrt::clock::Wall::Now();

    if (now > lease_time) {
      kabort("UNIMPLENTED: DHCP lease expired\n");
    }

    if (now > renewal_time) {
      kabort("UNIMPLEMENTED: DHCP renewal\n");
    }

    if (now > rebind_time) {
      kabort("UNIMPLEMENTED: DHCP rebind\n");
    }
    break;
  }
  default: { break; }
  }
}

void
ebbrt::NetworkManager::Interface::DhcpHandleAck(const DhcpMessage& message) {
  timer->Stop(dhcp_pcb_);

  auto now = ebbrt::clock::Wall::Now();

  auto lease_secs_opt = DhcpGetOptionLong(message, kDhcpOptionLeaseTime);

  auto lease_time = lease_secs_opt
                        ? std::chrono::seconds(ntohl(*lease_secs_opt))
                        : std::chrono::seconds::zero();

  dhcp_pcb_.lease_time = now + lease_time;

  auto renew_secs_opt = DhcpGetOptionLong(message, kDhcpOptionRenewalTime);

  auto renewal_time = renew_secs_opt
                          ? std::chrono::seconds(ntohl(*renew_secs_opt))
                          : lease_time / 2;

  dhcp_pcb_.renewal_time = now + renewal_time;

  auto rebind_secs_opt = DhcpGetOptionLong(message, kDhcpOptionRebindingTime);

  auto rebind_time = rebind_secs_opt
                         ? std::chrono::seconds(ntohl(*rebind_secs_opt))
                         : lease_time;

  dhcp_pcb_.rebind_time = now + rebind_time;

  timer->Start(dhcp_pcb_, renewal_time, /* repeat = */ false);

  auto addr = new ItfAddress();

  const auto& print_addr = message.yiaddr.toArray();
  kprintf("Dhcp Complete: %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 "\n",
          print_addr[0], print_addr[1], print_addr[2], print_addr[3]);

  addr->address = message.yiaddr;

  auto netmask_opt = DhcpGetOptionLong(message, kDhcpOptionSubnetMask);
  kassert(netmask_opt);

  addr->netmask = *netmask_opt;

  auto gw_opt = DhcpGetOptionLong(message, kDhcpOptionRouter);
  kassert(gw_opt);

  addr->gateway = *gw_opt;

  SetAddress(std::unique_ptr<ItfAddress>(addr));

  DhcpSetState(DhcpPcb::State::kBound);

  dhcp_pcb_.complete.SetValue();
}

void ebbrt::NetworkManager::Interface::ReceiveDhcp(
    Ipv4Address from_addr, uint16_t from_port, std::unique_ptr<MutIOBuf> buf) {
  auto len = buf->ComputeChainDataLength();

  if (len < sizeof(DhcpMessage))
    return;

  auto dp = buf->GetDataPointer();
  auto& dhcp_message = dp.Get<DhcpMessage>();

  if (dhcp_message.op != kDhcpOpReply)
    return;

  std::lock_guard<std::mutex> guard(dhcp_pcb_.lock);
  if (ntohl(dhcp_message.xid) != dhcp_pcb_.xid)
    return;

  auto msg_type = DhcpGetOptionByte(dhcp_message, kDhcpOptionMessageType);
  if (!msg_type)
    return;

  switch (*msg_type) {
  case kDhcpOptionMessageTypeOffer:
    if (dhcp_pcb_.state != DhcpPcb::State::kSelecting)
      return;

    DhcpHandleOffer(dhcp_message);
    break;
  case kDhcpOptionMessageTypeAck:
    if (dhcp_pcb_.state != DhcpPcb::State::kRequesting)
      return;

    DhcpHandleAck(dhcp_message);
    break;
  }
}

void
ebbrt::NetworkManager::Interface::ReceiveArp(EthernetHeader& eth_header,
                                             std::unique_ptr<MutIOBuf> buf) {
  auto packet_len = buf->ComputeChainDataLength();

  if (packet_len < sizeof(ArpPacket))
    return;

  auto dp = buf->GetMutDataPointer();
  auto& arp_packet = dp.Get<ArpPacket>();

  // RFC 826: do we have the hardware type and do we speak the protocol?
  if (ntohs(arp_packet.htype) != kHTypeEth ||
      ntohs(arp_packet.ptype) != kEthTypeIp)
    return;

  auto entry = network_manager->arp_cache_.find(arp_packet.spa);

  if (entry) {
    // RFC 826: If entry is found, update it
    entry->SetAddr(new EthernetAddress(arp_packet.sha));
  }

  // Am I the target address?
  auto addr = Address();
  if (addr && addr->address == arp_packet.tpa) {
    if (!entry) {
      // RFC 826: If target address matches ours, and we didn't update the
      // entry, create it
      std::lock_guard<std::mutex> guard(network_manager->arp_write_lock_);

      // double check for entry
      entry = network_manager->arp_cache_.find(arp_packet.spa);
      if (entry) {
        // RFC 826: If entry is found, update it
        entry->SetAddr(new EthernetAddress(arp_packet.sha));
      } else {
        network_manager->arp_cache_.insert(
            *new ArpEntry(arp_packet.spa, new EthernetAddress(arp_packet.sha)));
      }
    }

    if (ntohs(arp_packet.oper) == kArpRequest) {
      // RFC 826: If we received an arp request, send back a reply

      // We just reuse this packet, rewriting it
      eth_header.dst = eth_header.src;
      eth_header.src = MacAddress();
      arp_packet.tha = arp_packet.sha;
      arp_packet.tpa = arp_packet.spa;
      arp_packet.sha = MacAddress();
      arp_packet.spa = addr->address;
      arp_packet.oper = htons(kArpReply);

      buf->Retreat(sizeof(EthernetHeader));

      Send(std::move(buf));
    }
  }
}

void
ebbrt::NetworkManager::Interface::ReceiveIp(EthernetHeader& eth_header,
                                            std::unique_ptr<MutIOBuf> buf) {
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

  if (unlikely(ip_header.ComputeChecksum() != 0))
    return;

  auto addr = Address();
  // If the protocol is not UDP (for dhcp purposes) and we don't have an address
  // or the address is not broadcast and not our address, then drop
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
    ReceiveUdp(ip_header, std::move(buf));
    break;
  }
  case kIpProtoTCP: {
    ReceiveTcp(ip_header, std::move(buf));
    break;
  }
  }
}

void
ebbrt::NetworkManager::Interface::ReceiveIcmp(EthernetHeader& eth_header,
                                              Ipv4Header& ip_header,
                                              std::unique_ptr<MutIOBuf> buf) {
  auto packet_len = buf->ComputeChainDataLength();

  if (unlikely(packet_len < sizeof(IcmpHeader)))
    return;

  auto dp = buf->GetMutDataPointer();
  auto& icmp_header = dp.Get<IcmpHeader>();

  // checksum
  if (IpCsum(*buf))
    return;

  // if echo_request, send reply
  if (icmp_header.type == kIcmpEchoRequest) {
    auto tmp = ip_header.dst;
    ip_header.dst = ip_header.src;
    ip_header.src = tmp;

    icmp_header.type = kIcmpEchoReply;

    // adjust checksum
    auto addend = static_cast<uint16_t>(kIcmpEchoRequest) << 8;
    // check wrap of checksum
    if (icmp_header.checksum >= htons(0xffff - addend)) {
      icmp_header.checksum += htons(addend) + 1;
    } else {
      icmp_header.checksum += htons(addend);
    }

    ip_header.ttl = kIpDefaultTtl;
    ip_header.chksum = 0;
    ip_header.chksum = ip_header.ComputeChecksum();

    buf->Retreat(ip_header.HeaderLength());
    EthArpSend(kEthTypeIp, ip_header, std::move(buf));
  }
}

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

  if (!entry || !entry->reading.load(std::memory_order_consume))
    return;

  buf->Advance(sizeof(UdpHeader));

  entry->func(ip_header.src, ntohs(udp_header.src_port), std::move(buf));
}

void
ebbrt::NetworkManager::Interface::EthArpSend(uint16_t proto,
                                             const Ipv4Header& ip_header,
                                             std::unique_ptr<MutIOBuf> buf) {
  buf->Retreat(sizeof(EthernetHeader));
  auto dp = buf->GetMutDataPointer();
  auto& eth_header = dp.Get<EthernetHeader>();
  Ipv4Address local_dest = ip_header.dst;
  auto addr = Address();
  if (ip_header.dst == Ipv4Address::Broadcast() ||
      (addr && addr->isBroadcast(ip_header.dst))) {
    eth_header.dst.fill(0xff);
    eth_header.src = MacAddress();
    eth_header.type = htons(proto);
    Send(std::move(buf));
  } else if (ip_header.dst.isMulticast()) {
    kabort("UNIMPLEMENTED: Multicast send\n");
  } else if (addr) {
    // on the outside network
    if (!addr->isLocalNetwork(ip_header.dst) && !ip_header.dst.isLinkLocal()) {
      local_dest = addr->gateway;
    }

    auto send_func = MoveBind([this, proto](std::unique_ptr<MutIOBuf> buf,
                                            EthernetAddress addr) {
                                auto dp = buf->GetMutDataPointer();
                                auto& eth_header = dp.Get<EthernetHeader>();
                                eth_header.dst = addr;
                                eth_header.src = MacAddress();
                                eth_header.type = htons(proto);
                                Send(std::move(buf));
                              },
                              std::move(buf));

    // look up local_dest in arp cache
    auto entry = network_manager->arp_cache_.find(local_dest);
    if (!entry) {
      // no entry, lock and check again
      network_manager->arp_write_lock_.lock();
      entry = network_manager->arp_cache_.find(local_dest);
      if (!entry) {
        // create entry
        auto& new_arp_entry = *new ArpEntry(local_dest);
        network_manager->arp_cache_.insert(new_arp_entry);
        network_manager->arp_write_lock_.unlock();
        EthArpRequest(new_arp_entry);
        new_arp_entry.queue.Push(std::move(send_func));
        return;
      } else {
        network_manager->arp_write_lock_.unlock();
      }
    } else {
      // entry found
      auto eth_addr = entry->hwaddr.get();
      if (!eth_addr) {
        // no addr yet, pending request
        entry->queue.Push(std::move(send_func));
      } else {
        // got addr, just send right away
        send_func(*eth_addr);
      }
    }
  }
}

void ebbrt::NetworkManager::Interface::EthArpRequest(ArpEntry& entry) {
  auto buf = std::unique_ptr<MutUniqueIOBuf>(
      new MutUniqueIOBuf(sizeof(EthernetHeader) + sizeof(ArpPacket)));
  auto dp = buf->GetMutDataPointer();
  auto& eth_header = dp.Get<EthernetHeader>();
  auto& arp_packet = dp.Get<ArpPacket>();

  eth_header.dst = BroadcastMAC;
  eth_header.src = MacAddress();
  eth_header.type = htons(kEthTypeArp);

  arp_packet.htype = htons(kHTypeEth);
  arp_packet.ptype = htons(kPTypeIp);
  arp_packet.hlen = kEthHwAddrLen;
  arp_packet.plen = 4;
  arp_packet.oper = htons(kArpRequest);
  arp_packet.sha = MacAddress();

  auto addr = Address();
  if (addr)
    arp_packet.spa = addr->address;

  arp_packet.tha = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  arp_packet.tpa = entry.paddr;

  Send(std::move(buf));
}

const ebbrt::EthernetAddress& ebbrt::NetworkManager::Interface::MacAddress() {
  return ether_dev_.GetMacAddress();
}

void ebbrt::NetworkManager::Interface::Send(std::unique_ptr<IOBuf> b) {
  ether_dev_.Send(std::move(b));
}

void ebbrt::NetworkManager::UdpPcb::UdpEntryDeleter::operator()(UdpEntry* e) {
  if (e->port) {
    network_manager->udp_port_allocator_->Free(e->port);
    std::lock_guard<std::mutex> guard(network_manager->udp_write_lock_);
    network_manager->udp_pcbs_.erase(*e);
    e->reading.store(false, std::memory_order_release);
  }
  event_manager->DoRcu([e]() { delete e; });
}

uint16_t ebbrt::NetworkManager::UdpPcb::Bind(uint16_t port) {
  if (!port) {
    auto ret = network_manager->udp_port_allocator_->Allocate();
    if (!ret)
      throw std::runtime_error("Failed to allocate ephemeral port");

    port = *ret;
  }
  // TODO(dschatz): check port is free
  entry_->port = port;
  {
    std::lock_guard<std::mutex> guard(network_manager->udp_write_lock_);
    network_manager->udp_pcbs_.insert(*entry_);
  }
  entry_->reading.store(true, std::memory_order_release);
  return port;
}

void ebbrt::NetworkManager::UdpPcb::Receive(MovableFunction<
    void(Ipv4Address, uint16_t, std::unique_ptr<MutIOBuf>)> func) {
  entry_->func = std::move(func);
}

void ebbrt::NetworkManager::UdpPcb::SendTo(Ipv4Address addr, uint16_t port,
                                           std::unique_ptr<IOBuf> buf) {
  auto itf = network_manager->IpRoute(addr);

  if (!itf)
    return;

  itf->SendUdp(*this, addr, port, std::move(buf));
}

void ebbrt::NetworkManager::Interface::SendUdp(UdpPcb& pcb, Ipv4Address addr,
                                               uint16_t port,
                                               std::unique_ptr<IOBuf> buf) {
  auto itf_addr = Address();
  Ipv4Address src_addr;
  if (!itf_addr) {
    src_addr = Ipv4Address::Any();
  } else {
    src_addr = itf_addr->address;
  }

  auto src_port = pcb.entry_->port;
  // Bind port if not already bound
  if (!src_port)
    throw std::runtime_error("Send on unbound pcb");

  auto header_buf = std::unique_ptr<MutUniqueIOBuf>(new MutUniqueIOBuf(
      sizeof(UdpHeader) + sizeof(Ipv4Header) + sizeof(EthernetHeader)));

  header_buf->Advance(sizeof(Ipv4Header) + sizeof(EthernetHeader));

  auto dp = header_buf->GetMutDataPointer();
  auto& udp_header = dp.Get<UdpHeader>();
  udp_header.src_port = htons(src_port);
  udp_header.dst_port = htons(port);
  udp_header.length = htons(buf->ComputeChainDataLength() + sizeof(UdpHeader));
  udp_header.checksum = 0;

  header_buf->AppendChain(std::move(buf));

  auto csum = IpPseudoCsum(*header_buf, kIpProtoUDP, src_addr, addr);

  if (unlikely(csum == 0x0000))
    csum = 0xffff;

  udp_header.checksum = csum;

  kassert(IpPseudoCsum(*header_buf, kIpProtoUDP, src_addr, addr) == 0);

  SendIp(std::move(header_buf), src_addr, addr, kIpProtoUDP);
}

void ebbrt::NetworkManager::Interface::SendIp(std::unique_ptr<MutIOBuf> buf,
                                              Ipv4Address src, Ipv4Address dst,
                                              uint8_t proto) {
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
  ih.chksum = ih.ComputeChecksum();

  kassert(ih.ComputeChecksum() == 0);

  EthArpSend(kEthTypeIp, ih, std::move(buf));
}

ebbrt::Future<void> ebbrt::NetworkManager::StartDhcp() {
  if (interface_)
    return interface_->StartDhcp();

  return MakeFailedFuture<void>(
      std::make_exception_ptr(std::runtime_error("No interface to DHCP")));
}

ebbrt::NetworkManager::Interface*
ebbrt::NetworkManager::IpRoute(Ipv4Address dest) {
  // Find first matching interface
  if (!interface_)
    return nullptr;

  return &*interface_;
}

void ebbrt::NetworkManager::TcpPcb::TcpEntryDeleter::operator()(TcpEntry* e) {
  if (e->port) {
    network_manager->tcp_port_allocator_->Free(e->port);
    std::lock_guard<std::mutex> guard(network_manager->tcp_write_lock_);
    network_manager->tcp_pcbs_.erase(*e);
  }
  event_manager->DoRcu([e]() { delete e; });
}

uint16_t ebbrt::NetworkManager::TcpPcb::Bind(uint16_t port) {
  if (!port) {
    auto ret = network_manager->tcp_port_allocator_->Allocate();
    if (!ret)
      throw std::runtime_error("Failed to allocate ephemeral port");

    port = *ret;
  }
  // TODO(dschatz): check port is free
  entry_->port = port;
  {
    std::lock_guard<std::mutex> guard(network_manager->tcp_write_lock_);
    network_manager->tcp_pcbs_.insert(*entry_);
  }
  return port;
}

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

  TcpInfo info = {.src_port = ntohs(tcp_header.src_port),
                  .dst_port = ntohs(tcp_header.dst_port),
                  .seqno = ntohl(tcp_header.seqno),
                  .ackno = ntohl(tcp_header.ackno),
                  .tcplen =
                      packet_len - tcp_header.HdrLen() +
                      ((tcp_header.Flags() & (kTcpFin | kTcpSyn)) ? 1 : 0)};

  auto entry = network_manager->tcp_pcbs_.find(ntohs(tcp_header.dst_port));

  if (!entry) {
    TcpReset(info.ackno, info.seqno + info.tcplen, ih.dst, ih.src,
             info.dst_port, info.src_port);
    return;
  } else {
    // Check connected pcbs

    TcpListenInput(ih, tcp_header, info, std::move(buf));
  }
}

void ebbrt::NetworkManager::Interface::TcpListenInput(
    const Ipv4Header& ih, const TcpHeader& th, const TcpInfo& info,
    std::unique_ptr<MutIOBuf> buf) {
  auto flags = th.Flags();

  if (flags & kTcpRst)
    return;

  if (flags & kTcpAck) {
    TcpReset(info.ackno, info.seqno + info.tcplen, ih.dst, ih.src,
             info.dst_port, info.src_port);
  } else if (flags & kTcpSyn) {
    // TcpPcb new_pcb;

    // auto& entry = *(new_pcb.entry_);
  }
}

void ebbrt::NetworkManager::Interface::TcpReset(uint32_t seqno, uint32_t ackno,
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

// void ebbrt::NetworkManager::Interface::Send(std::unique_ptr<IOBuf>&& b) {
//   ether_dev_.Send(std::move(b));
// }

// #include <cstring>

// #include <lwip/dhcp.h>
// #include <lwip/init.h>
// #include <lwip/stats.h>
// #include <lwip/sys.h>
// #include <lwip/tcp.h>
// #include <lwip/timers.h>
// #include <netif/etharp.h>

// #include <ebbrt/BootFdt.h>
// #include <ebbrt/Clock.h>
// #include <ebbrt/Debug.h>
// #include <ebbrt/EventManager.h>
// #include <ebbrt/ExplicitlyConstructed.h>
// #include <ebbrt/Rdtsc.h>
// #include <ebbrt/Timer.h>

// namespace {
// ebbrt::ExplicitlyConstructed<ebbrt::NetworkManager> the_manager;
// }

// void ebbrt::NetworkManager::Init() {
//   the_manager.construct();
//   lwip_init();
//   timer->Start(std::chrono::milliseconds(10),
//                []() {
//                  for (auto& itf : network_manager->interfaces_) {
//                    itf.Poll();
//                  }
//                  sys_check_timeouts();
//                },
//                /* repeat = */ true);
// }

// ebbrt::NetworkManager& ebbrt::NetworkManager::HandleFault(EbbId id) {
//   kassert(id == kNetworkManagerId);
//   auto& ref = *the_manager;
//   EbbRef<NetworkManager>::CacheRef(id, ref);
//   return ref;
// }

// ebbrt::NetworkManager::Interface&
// ebbrt::NetworkManager::NewInterface(EthernetDevice& ether_dev) {
//   interfaces_.emplace_back(ether_dev, interfaces_.size());
//   return interfaces_[interfaces_.size() - 1];
// }

// ebbrt::NetworkManager::Interface& ebbrt::NetworkManager::FirstInterface() {
//   if (interfaces_.empty())
//     throw std::runtime_error("No Interfaces");
//   return interfaces_.front();
// }

// #ifndef __EBBRT_ENABLE_STATIC_IP__
// namespace {
// ebbrt::EventManager::EventContext* context;
// }
// #endif

// void ebbrt::NetworkManager::AcquireIPAddress() {
//   kbugon(interfaces_.size() == 0, "No network interfaces identified!\n");
//   auto netif = &interfaces_[0].netif_;
//   netif_set_default(netif);
// #ifdef __EBBRT_ENABLE_STATIC_IP__
//   const auto& mac_addr = interfaces_[0].MacAddress();
//   auto fdt = boot_fdt::Get();
//   auto net = static_cast<uint8_t>(
//       fdt.GetProperty16(fdt.GetNodeOffset("/runtime"), "net"));
//   ip_addr_t addr;
//   IP4_ADDR(&addr, 10, net, net,
//            mac_addr[5]);  // set addr to 10.net.net.last_mac_octet
//   ip_addr_t netmask;
//   IP4_ADDR(&netmask, 255, 255, 255, 0);  // set netmask to 255.255.255.0
//   ip_addr_t gw;
//   IP4_ADDR(&gw, 10, net, net, 1);  // set gw to 10.net.net.1
//   netif_set_addr(netif, &addr, &netmask, &gw);
//   netif_set_up(netif);
// #else
//   dhcp_start(netif);
//   context = new EventManager::EventContext;
//   event_manager->SaveContext(*context);
// #endif
// }

// namespace {
// err_t EthOutput(struct netif* netif, struct pbuf* p) {
//   auto itf = static_cast<ebbrt::NetworkManager::Interface*>(netif->state);

// #if ETH_PAD_SIZE
//   pbuf_header(p, -ETH_PAD_SIZE);
// #endif

//   if (p == nullptr)
//     return ERR_OK;

//   pbuf_ref(p);
//   auto b = ebbrt::IOBuf::TakeOwnership(p->payload, p->len,
//                                        [p](void* pointer) { pbuf_free(p); });
//   // TODO(dschatz): avoid this copy
//   // auto b = ebbrt::IOBuf::Create(p->len);
//   // std::memcpy(b->WritableData(), p->payload, p->len);

//   for (struct pbuf* q = p->next; q != nullptr; q = q->next) {
//     auto nbuf =
//         ebbrt::IOBuf::TakeOwnership(q->payload, q->len, [](void* pointer)
// {});
//     // auto nbuf = ebbrt::IOBuf::Create(q->len);
//     // std::memcpy(nbuf->WritableData(), q->payload, q->len);
//     b->Prev()->AppendChain(std::move(nbuf));
//   }
//   itf->Send(std::move(b));

// #if ETH_PAD_SIZE
//   pbuf_header(p, ETH_PAD_SIZE);
// #endif

//   LINK_STATS_INC(link.xmit);

//   return ERR_OK;
// }

// err_t EthInit(struct netif* netif) {
//   auto itf = static_cast<ebbrt::NetworkManager::Interface*>(netif->state);
//   netif->hwaddr_len = 6;
//   memcpy(netif->hwaddr, itf->MacAddress().data(), 6);
//   netif->mtu = 1500;
//   netif->name[0] = 'e';
//   netif->name[1] = 'n';
//   netif->output = etharp_output;
//   netif->linkoutput = EthOutput;
//   netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
// NETIF_FLAG_LINK_UP;
//   return ERR_OK;
// }

// void StatusCallback(struct netif* netif) {
//   ebbrt::kprintf("IP address: %" U16_F ".%" U16_F ".%" U16_F ".%" U16_F "\n",
//                  ip4_addr1_16(&netif->ip_addr),
// ip4_addr2_16(&netif->ip_addr),
//                  ip4_addr3_16(&netif->ip_addr),
// ip4_addr4_16(&netif->ip_addr));
// #ifndef __EBBRT_ENABLE_STATIC_IP__
//   ebbrt::event_manager->ActivateContext(std::move(*context));
//   delete context;
// #endif
// }
// }  // namespace

// ebbrt::NetworkManager::Interface::Interface(EthernetDevice& ether_dev,
//                                             size_t idx)
//     : ether_dev_(ether_dev), idx_(idx) {
//   if (netif_add(&netif_, nullptr, nullptr, nullptr, static_cast<void*>(this),
//                 EthInit, ethernet_input) == nullptr) {
//     throw std::runtime_error("Failed to create network interface");
//   }
//   netif_set_status_callback(&netif_, StatusCallback);
// }

// void ebbrt::NetworkManager::Interface::Poll() { return ether_dev_.Poll(); }

// const std::array<char, 6>& ebbrt::NetworkManager::Interface::MacAddress() {
//   return ether_dev_.GetMacAddress();
// }

// uint32_t ebbrt::NetworkManager::Interface::IPV4Addr() {
//   return netif_.ip_addr.addr;
// }

// void ebbrt::NetworkManager::Interface::Send(std::unique_ptr<IOBuf>&& b) {
//   ether_dev_.Send(std::move(b));
// }

// struct pbuf_custom_wrapper {
//   pbuf_custom p_custom;
//   std::unique_ptr<ebbrt::IOBuf> buf;
// };

// void ebbrt::NetworkManager::Interface::Receive(std::unique_ptr<IOBuf>&& buf)
// {
//   kbugon(buf->CountChainElements() > 1, "Handle chained buffer\n");
//   auto data = buf->WritableData() - ETH_PAD_SIZE;
//   auto len = buf->ComputeChainDataLength() + ETH_PAD_SIZE;

//   auto p_wrapper = new pbuf_custom_wrapper();
//   p_wrapper->buf = std::move(buf);
//   p_wrapper->p_custom.custom_free_function = [](pbuf* p) {
//     auto wrapper = reinterpret_cast<pbuf_custom_wrapper*>(p);
//     delete wrapper;
//   };
//   auto p = pbuf_alloced_custom(PBUF_RAW, len, PBUF_REF, &p_wrapper->p_custom,
//                                data, len);
//   kbugon(p == nullptr, "Failed to allocate pbuf\n");

//   // event_manager->SpawnLocal([p, this]() { netif_.input(p, &netif_); });
//   netif_.input(p, &netif_);
// }

// extern "C" void lwip_printf(const char* fmt, ...) {
//   va_list ap;
//   va_start(ap, fmt);
//   ebbrt::kvprintf(fmt, ap);
//   va_end(ap);
// }

// extern "C" void lwip_assert(const char* fmt, ...) {
//   va_list ap;
//   va_start(ap, fmt);
//   ebbrt::kvprintf(fmt, ap);
//   va_end(ap);
//   ebbrt::kprintf("\n");
//   ebbrt::kabort();
// }

// extern "C" uint32_t lwip_rand() { return ebbrt::rdtsc() % 0xFFFFFFFF; }

// u32_t sys_now() {
//   return std::chrono::duration_cast<std::chrono::milliseconds>(
//              ebbrt::clock::Wall::Now().time_since_epoch()).count();
// }

// void ebbrt::NetworkManager::TcpPcb::Deleter(struct tcp_pcb* object) {
//   if (object != nullptr)
//     tcp_close(object);
// }

// ebbrt::NetworkManager::TcpPcb::TcpPcb()
//     : pcb_(nullptr, &Deleter), sent_(0), next_(0) {
//   auto pcb = tcp_new();
//   if (pcb == nullptr) {
//     throw std::bad_alloc();
//   }
//   pcb_ =
//       std::unique_ptr<struct tcp_pcb, void (*)(struct tcp_pcb*)>(pcb,
// &Deleter);
//   tcp_arg(pcb_.get(), static_cast<void*>(this));
// }

// ebbrt::NetworkManager::TcpPcb::TcpPcb(struct tcp_pcb* pcb)
//     : pcb_(pcb, &Deleter), sent_(0), next_(0) {
//   tcp_arg(pcb_.get(), static_cast<void*>(this));
//   tcp_sent(pcb_.get(), SentHandler);
// }

// ebbrt::NetworkManager::TcpPcb::TcpPcb(TcpPcb&& other)
//     : pcb_(std::move(other.pcb_)),
//       accept_callback_(std::move(other.accept_callback_)),
//       connect_promise_(std::move(other.connect_promise_)),
//       receive_callback_(std::move(other.receive_callback_)),
//       sent_(std::move(other.sent_)), next_(std::move(other.next_)),
//       ack_queue_(std::move(other.ack_queue_)) {
//   tcp_arg(pcb_.get(), static_cast<void*>(this));
// }

// ebbrt::NetworkManager::TcpPcb& ebbrt::NetworkManager::TcpPcb::
// operator=(TcpPcb&& other) {
//   pcb_ = std::move(other.pcb_);
//   accept_callback_ = std::move(other.accept_callback_);
//   connect_promise_ = std::move(other.connect_promise_);
//   receive_callback_ = std::move(other.receive_callback_);
//   sent_ = std::move(other.sent_);
//   next_ = std::move(other.next_);
//   ack_queue_ = std::move(other.ack_queue_);

//   tcp_arg(pcb_.get(), static_cast<void*>(this));

//   return *this;
// }

// void ebbrt::NetworkManager::TcpPcb::Bind(uint16_t port) {
//   auto ret = tcp_bind(pcb_.get(), IP_ADDR_ANY, port);
//   if (ret != ERR_OK) {
//     throw std::runtime_error("Bind failed\n");
//   }
// }

// void ebbrt::NetworkManager::TcpPcb::Listen() {
//   auto pcb = tcp_listen(pcb_.get());
//   if (pcb == NULL) {
//     throw std::bad_alloc();
//   }
//   pcb_.release();
//   pcb_.reset(pcb);
// }

// void ebbrt::NetworkManager::TcpPcb::ListenWithBacklog(uint8_t backlog) {
//   auto pcb = tcp_listen_with_backlog(pcb_.get(), backlog);
//   if (pcb == NULL) {
//     throw std::bad_alloc();
//   }
//   pcb_.release();
//   pcb_.reset(pcb);
// }

// void
// ebbrt::NetworkManager::TcpPcb::Accept(std::function<void(TcpPcb)> callback) {
//   accept_callback_ = std::move(callback);
//   tcp_accept(pcb_.get(), Accept_Handler);
// }

// ebbrt::Future<void> ebbrt::NetworkManager::TcpPcb::Connect(struct ip_addr*
// ip,
//                                                            uint16_t port) {
//   auto err = tcp_connect(pcb_.get(), ip, port, Connect_Handler);
//   if (err != ERR_OK) {
//     throw std::bad_alloc();
//   }
//   return connect_promise_.GetFuture();
// }

// err_t ebbrt::NetworkManager::TcpPcb::Accept_Handler(void* arg,
//                                                     struct tcp_pcb* newpcb,
//                                                     err_t err) {
//   kassert(err == ERR_OK);
//   auto listening_pcb = static_cast<TcpPcb*>(arg);
//   listening_pcb->accept_callback_(TcpPcb(newpcb));
//   tcp_accepted(listening_pcb->pcb_.get());
//   return ERR_OK;
// }

// err_t ebbrt::NetworkManager::TcpPcb::Connect_Handler(void* arg,
//                                                      struct tcp_pcb* pcb,
//                                                      err_t err) {
//   kassert(err == ERR_OK);
//   auto pcb_s = static_cast<TcpPcb*>(arg);
//   kassert(pcb_s->pcb_.get() == pcb);
//   tcp_sent(pcb_s->pcb_.get(), SentHandler);
//   pcb_s->connect_promise_.SetValue();
//   return ERR_OK;
// }

// void ebbrt::NetworkManager::TcpPcb::Receive(
//     std::function<void(TcpPcb&, std::unique_ptr<IOBuf>&&)> callback) {
//   if (!receive_callback_) {
//     tcp_recv(pcb_.get(), Receive_Handler);
//   }
//   receive_callback_ = std::move(callback);
// }

// err_t ebbrt::NetworkManager::TcpPcb::Receive_Handler(void* arg,
//                                                      struct tcp_pcb* pcb,
//                                                      struct pbuf* p,
//                                                      err_t err) {
//   kassert(err == ERR_OK);
//   auto pcb_s = static_cast<TcpPcb*>(arg);
//   kassert(pcb_s->pcb_.get() == pcb);
//   if (p == nullptr) {
//     pcb_s->receive_callback_(*pcb_s, IOBuf::Create(0));
//   } else {
//     auto b = IOBuf::TakeOwnership(p->payload, p->len,
//                                   [p](void* pointer) { pbuf_free(p); });
//     for (auto q = p->next; q != nullptr; q = q->next) {
//       auto n = IOBuf::TakeOwnership(q->payload, q->len, [](void* pointer)
// {});
//       b->Prev()->AppendChain(std::move(n));
//     }
//     pcb_s->receive_callback_(*pcb_s, std::move(b));
//     tcp_recved(pcb_s->pcb_.get(), p->tot_len);
//   }
//   return ERR_OK;
// }  // NOLINT

// size_t ebbrt::NetworkManager::TcpPcb::InternalSend(
//     const std::unique_ptr<IOBuf>& data) {
//   size_t ret = 0;
//   for (auto& buf : *data) {
//     while (buf.Length() != 0) {
//       auto sndbuf = tcp_sndbuf(pcb_.get());
//       if (unlikely(sndbuf == 0))
//         return ret;
//       auto sz = buf.Length();
//       sz = sz > UINT16_MAX ? UINT16_MAX : sz;
//       sz = sz > sndbuf ? sndbuf : sz;
//       uint8_t flag = 0;
//       if (sz < sndbuf && buf.Next() != &*data) {
//         // we expect to write more almost immediately if there is room in the
//         // send buf and this is not the last element of the chain
//         flag = TCP_WRITE_FLAG_MORE;
//       }
//       auto err =
//           tcp_write(pcb_.get(), const_cast<uint8_t*>(buf.Data()), sz, flag);
//       kassert(err == ERR_OK);
//       buf.Advance(sz);
//       ret += sz;
//     }
//   }
//   return ret;
// }

// ebbrt::Future<void>
// ebbrt::NetworkManager::TcpPcb::Send(std::unique_ptr<IOBuf>&& data) {
//   auto len = data->ComputeChainDataLength();
//   size_t written = 0;
//   if (likely(queued_bufs_.empty())) {
//     written = InternalSend(data);
//   }
//   // The IOBuf is stored in the ack map until LWIP tells us it has been sent
//   next_ += len;
//   ack_queue_.emplace(next_, Promise<void>(), std::move(data));
//   auto& p = ack_queue_.back();
//   auto& d = std::get<2>(p);
//   if (written < len) {
//     queued_bufs_.emplace(std::ref(d));
//   }
//   return std::get<1>(p).GetFuture();
// }

// void ebbrt::NetworkManager::TcpPcb::EnableNagle() { tcp_nagle_enable(pcb_); }
// void ebbrt::NetworkManager::TcpPcb::DisableNagle() { tcp_nagle_disable(pcb_);
// }
// bool ebbrt::NetworkManager::TcpPcb::NagleDisabled() {
//   return tcp_nagle_disabled(pcb_);
// }

// err_t ebbrt::NetworkManager::TcpPcb::SentHandler(void* arg, struct tcp_pcb*
// pcb,
//                                                  uint16_t len) {
//   auto pcb_s = static_cast<TcpPcb*>(arg);
//   kassert(pcb_s->pcb_.get() == pcb);
//   pcb_s->sent_ += len;
//   while (!pcb_s->ack_queue_.empty() &&
//          std::get<0>(pcb_s->ack_queue_.front()) <= pcb_s->sent_) {
//     std::get<1>(pcb_s->ack_queue_.front()).SetValue();
//     pcb_s->ack_queue_.pop();
//   }
//   while (!pcb_s->queued_bufs_.empty()) {
//     auto& buf = pcb_s->queued_bufs_.front().get();
//     auto buf_len = buf->ComputeChainDataLength();
//     auto written = pcb_s->InternalSend(buf);
//     if (written < buf_len)
//       break;
//     pcb_s->queued_bufs_.pop();
//   }
//   return ERR_OK;
// }

// ip_addr_t ebbrt::NetworkManager::TcpPcb::GetRemoteAddress() const {
//   return pcb_->remote_ip;
// }

// uint16_t ebbrt::NetworkManager::TcpPcb::GetRemotePort() const {
//   return pcb_->remote_port;
// }
