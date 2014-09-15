//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Net.h>

#include <ebbrt/UniqueIOBuf.h>

/// Send an Ethernet packet
void
ebbrt::NetworkManager::Interface::EthArpSend(uint16_t proto,
                                             const Ipv4Header& ip_header,
                                             std::unique_ptr<MutIOBuf> buf) {
  buf->Retreat(sizeof(EthernetHeader));
  auto dp = buf->GetMutDataPointer();
  auto& eth_header = dp.Get<EthernetHeader>();
  Ipv4Address local_dest = ip_header.dst;
  auto addr = Address();
  // If the destination is broadcast or at least broadcast on this segment, send
  // it out with the broadcast MAC
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

    // lambda to send the packet given the destination MAC address
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
        // send arp request to populate this entry
        // TODO(dschatz): Need to set a timer to retry this
        EthArpRequest(new_arp_entry);
        // enqueue function to send this packet out when arp is fulfilled
        new_arp_entry.queue.Push(std::move(send_func));
        return;
      } else {
        network_manager->arp_write_lock_.unlock();
        // fall through
      }
    }
    kassert(entry);
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

// Receive an ARP packet
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

// Request an ARP reply for a given entry
void ebbrt::NetworkManager::Interface::EthArpRequest(ArpEntry& entry) {
  auto buf = MakeUniqueIOBuf(sizeof(EthernetHeader) + sizeof(ArpPacket));
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
