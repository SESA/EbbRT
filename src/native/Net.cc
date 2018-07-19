//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Net.h"

void ebbrt::NetworkManager::Init() {}

ebbrt::NetworkManager::Interface&
ebbrt::NetworkManager::NewInterface(EthernetDevice& ether_dev) {
  interface_.reset(new Interface(ether_dev));
  return *interface_;
}

void ebbrt::NetworkManager::Interface::Receive(std::unique_ptr<MutIOBuf> buf,
                                               uint64_t rxflag) {
  auto packet_len = buf->ComputeChainDataLength();

  // Drop packets that are too small
  if (packet_len <= sizeof(EthernetHeader))
    return;

  auto dp = buf->GetMutDataPointer();
  auto& eth_header = dp.Get<EthernetHeader>();

  buf->Advance(sizeof(EthernetHeader));

  switch (ntohs(eth_header.type)) {
  case kEthTypeIp: {
    ReceiveIp(eth_header, std::move(buf), rxflag);
    break;
  }
  case kEthTypeArp: {
    ReceiveArp(eth_header, std::move(buf));
    break;
  }
  }
}

const ebbrt::EthernetAddress& ebbrt::NetworkManager::Interface::MacAddress() {
  return ether_dev_.GetMacAddress();
}

void ebbrt::NetworkManager::Interface::Send(std::unique_ptr<IOBuf> b,
                                            PacketInfo pinfo) {
  ether_dev_.Send(std::move(b), std::move(pinfo));
}
