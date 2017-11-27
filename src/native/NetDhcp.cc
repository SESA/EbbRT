//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Net.h"

#include <cinttypes>

#include "../Timer.h"
#include "../UniqueIOBuf.h"
#include "Multiboot.h"
#include "Random.h"

ebbrt::Future<void> ebbrt::NetworkManager::StartDhcp() {
  kbugon(Cpu::GetMine() != 0, "Dhcp not started on core 0!");
  // Before DHCP, check if a static IP has been specified
  auto cmdline = std::string(ebbrt::multiboot::CmdLine());
  auto loc = cmdline.find("ipaddr=");
  if (loc != std::string::npos) {
    kprintf("Warning: Skipping DHCP, static IP detected\n");
    return MakeReadyFuture<void>();
  }
  if (interface_)
    return interface_->StartDhcp();

  return MakeFailedFuture<void>(
      std::make_exception_ptr(std::runtime_error("No interface to DHCP")));
}

ebbrt::Future<void> ebbrt::NetworkManager::Interface::StartDhcp() {
  dhcp_pcb_.udp_pcb.Bind(kDhcpClientPort);
  dhcp_pcb_.udp_pcb.Receive([this](Ipv4Address from_addr, uint16_t from_port,
                                   std::unique_ptr<MutIOBuf> buf) {
    ReceiveDhcp(from_addr, from_port, std::move(buf));
  });
  DhcpDiscover();
  return dhcp_pcb_.complete.GetFuture();
}

void ebbrt::NetworkManager::Interface::ReceiveDhcp(
    Ipv4Address from_addr, uint16_t from_port, std::unique_ptr<MutIOBuf> buf) {
  if (Cpu::GetMine() != 0) {
    event_manager->SpawnRemote(
        [ this, from_addr, from_port, buf = std::move(buf) ]() mutable {
          ReceiveDhcp(from_addr, from_port, std::move(buf));
        },
        0);
  } else {
    auto len = buf->ComputeChainDataLength();

    if (len < sizeof(DhcpMessage))
      return;

    auto dp = buf->GetDataPointer();
    auto& dhcp_message = dp.Get<DhcpMessage>();

    if (dhcp_message.op != kDhcpOpReply)
      return;

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
}

// The initial DHCP state, discover DHCP servers
void ebbrt::NetworkManager::Interface::DhcpDiscover() {
  DhcpSetState(DhcpPcb::State::kSelecting);
  auto buf = MakeUniqueIOBuf(sizeof(DhcpMessage), true);
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

// Received an offer from a server
void ebbrt::NetworkManager::Interface::DhcpHandleOffer(
    const DhcpMessage& dhcp_message) {
  auto server_id_opt = DhcpGetOptionLong(dhcp_message, kDhcpOptionServerId);
  if (!server_id_opt)
    return;

  dhcp_pcb_.last_offer = dhcp_message;

  timer->Stop(dhcp_pcb_);
  const auto& offered_ip = dhcp_message.yiaddr;

  DhcpSetState(DhcpPcb::State::kRequesting);

  // create response which accepts the offer
  auto buf = MakeUniqueIOBuf(sizeof(DhcpMessage), true);
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

// A server acked our request for the offered address
void ebbrt::NetworkManager::Interface::DhcpHandleAck(
    const DhcpMessage& message) {
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

void ebbrt::NetworkManager::Interface::DhcpPcb::Fire() {
  switch (state) {
  case DhcpPcb::State::kRequesting: {
    auto itf =
        boost::intrusive::get_parent_from_member(this, &Interface::dhcp_pcb_);
    itf->DhcpHandleOffer(last_offer);
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

// Write a DHCP option header indicating the type and length
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

void ebbrt::NetworkManager::Interface::DhcpOptionLong(DhcpMessage& message,
                                                      uint32_t value) {
  auto& len = dhcp_pcb_.options_len;
  kassert((len + 4) <= kDhcpOptionsLen);
  *reinterpret_cast<uint32_t*>(message.options.data() + len) = value;
  len += 4;
}

// Mark the end of the options section
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

// Transition state of DHCP
void ebbrt::NetworkManager::Interface::DhcpSetState(DhcpPcb::State state) {
  if (state != dhcp_pcb_.state) {
    dhcp_pcb_.state = state;
    dhcp_pcb_.tries = 0;
  }
}

// Common DHCP message initialization
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
