//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NETDHCP_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NETDHCP_H_

#include "NetIpAddress.h"

namespace ebbrt {
const constexpr uint16_t kDhcpClientPort = 68;
const constexpr uint16_t kDhcpServerPort = 67;

const constexpr size_t kDhcpOptionsLen = 60;

struct __attribute__((packed)) DhcpMessage {
  uint8_t op;
  uint8_t htype;
  uint8_t hlen;
  uint8_t hops;
  uint32_t xid;
  uint16_t secs;
  uint16_t flags;
  Ipv4Address ciaddr;
  Ipv4Address yiaddr;
  Ipv4Address siaddr;
  Ipv4Address giaddr;
  std::array<uint8_t, 16> chaddr;
  std::array<uint8_t, 64> sname;
  std::array<uint8_t, 128> file;
  uint32_t cookie;
  std::array<uint8_t, kDhcpOptionsLen> options;
};

const constexpr uint8_t kDhcpOpRequest = 1;
const constexpr uint8_t kDhcpOpReply = 2;

const constexpr uint8_t kDhcpHTypeEth = 1;

const constexpr uint8_t kDhcpHLenEth = 6;

const constexpr uint32_t kDhcpMagicCookie = 0x63825363;

const constexpr uint8_t kDhcpOptionPad = 0;
const constexpr uint8_t kDhcpOptionSubnetMask = 1;
const constexpr uint8_t kDhcpOptionRouter = 3;
const constexpr uint8_t kDhcpOptionBroadcast = 28;

const constexpr uint8_t kDhcpOptionRequestIp = 50;

const constexpr uint8_t kDhcpOptionLeaseTime = 51;

const constexpr uint8_t kDhcpOptionMessageType = 53;
const constexpr uint8_t kDhcpOptionMessageTypeDiscover = 1;
const constexpr uint8_t kDhcpOptionMessageTypeOffer = 2;
const constexpr uint8_t kDhcpOptionMessageTypeRequest = 3;
const constexpr uint8_t kDhcpOptionMessageTypeDecline = 4;
const constexpr uint8_t kDhcpOptionMessageTypeAck = 5;
const constexpr uint8_t kDhcpOptionMessageTypeNak = 6;
const constexpr uint8_t kDhcpOptionMessageTypeRelease = 7;
const constexpr uint8_t kDhcpOptionMessageTypeInform = 8;

const constexpr uint8_t kDhcpOptionServerId = 54;

const constexpr uint8_t kDhcpOptionParameterRequestList = 55;

const constexpr uint8_t kDhcpOptionRenewalTime = 58;
const constexpr uint8_t kDhcpOptionRebindingTime = 59;

const constexpr uint8_t kDhcpOptionEnd = 255;
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NETDHCP_H_
