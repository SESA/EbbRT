//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Net.h"

#include "NetChecksum.h"
#include "NetIcmp.h"
// Receive an ICMP packet. Currently, if we get a request (ping) we just send
// back a reply
void ebbrt::NetworkManager::Interface::ReceiveIcmp(
    EthernetHeader& eth_header, Ipv4Header& ip_header,
    std::unique_ptr<MutIOBuf> buf) {
  auto packet_len = buf->ComputeChainDataLength();

  if (unlikely(packet_len < sizeof(IcmpHeader)))
    return;

  auto dp = buf->GetMutDataPointer();
  auto& icmp_header = dp.Get<IcmpHeader>();

#ifndef __EBBRT_ENABLE_BAREMETAL_NIC__
  // software checksum
  if (IpCsum(*buf))
    return;
#endif

  // if echo_request, send reply
  if (icmp_header.type == kIcmpEchoRequest) {
    // we reuse the incoming packet to send the reply
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

    PacketInfo pinfo;
    pinfo.flags = 0;

#ifdef __EBBRT_ENABLE_BAREMETAL_NIC__
    // hardware ip checksum offload
    pinfo.flags |= PacketInfo::kNeedsIpCsum;
#else
    ip_header.chksum = ip_header.ComputeChecksum();
#endif

    buf->Retreat(ip_header.HeaderLength());

    EthArpSend(kEthTypeIp, ip_header, std::move(buf), pinfo);
  }
}
