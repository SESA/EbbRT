/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <cstdint>
#include <cstdlib>

#include "app/app.hpp"
#include "arch/inet.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/Ethernet/Ethernet.hpp"
#include "ebb/MessageManager/EthernetMessageManager.hpp"

#ifdef __ebbrt__
#include "lrt/bare/assert.hpp"
#endif

namespace {
  constexpr uint16_t MESSAGE_MANAGER_ETHERTYPE = 0x8812;
}

ebbrt::EbbRoot*
ebbrt::EthernetMessageManager::ConstructRoot()
{
  return new SharedRoot<EthernetMessageManager>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol("MessageManager",
                        ebbrt::EthernetMessageManager::ConstructRoot);
}

namespace {
  class MessageHeader {
  public:
    ebbrt::EbbId ebb;
  };
};

ebbrt::EthernetMessageManager::EthernetMessageManager(EbbId id) :
  MessageManager{id}
{
  auto addr = ethernet->MacAddress();
  std::copy(&addr[0], &addr[6], mac_addr_);
}

ebbrt::Buffer
ebbrt::EthernetMessageManager::Alloc(size_t size)
{
  auto header_size = sizeof(MessageHeader) + sizeof(Ethernet::Header);
  return ethernet->Alloc(header_size + size) + header_size;
}

void
ebbrt::EthernetMessageManager::Send(NetworkId to,
                                    EbbId id,
                                    Buffer buffer)
{
  auto header_size = sizeof(MessageHeader) + sizeof(Ethernet::Header);
  auto buf = buffer - header_size;
  auto eth_header = reinterpret_cast<Ethernet::Header*>(buf.data());
  std::copy(&to.mac_addr[0], &to.mac_addr[6], eth_header->destination);
  std::copy(&mac_addr_[0], &mac_addr_[6], eth_header->source);
  eth_header->ethertype = htons(MESSAGE_MANAGER_ETHERTYPE);

  auto msg_header_p = buf.data() + sizeof(Ethernet::Header);
  auto msg_header = reinterpret_cast<MessageHeader*>(msg_header_p);
  msg_header->ebb = id;

  ethernet->Send(std::move(buf));
}

void
ebbrt::EthernetMessageManager::StartListening()
{
  ethernet->Register(MESSAGE_MANAGER_ETHERTYPE,
                     [](Buffer buffer) {
                       auto eth_header = reinterpret_cast<const Ethernet::Header*>(buffer.data());
                       NetworkId id;
                       std::copy(&eth_header->source[0], &eth_header->source[6],
                                 id.mac_addr);
                       auto msg_header = reinterpret_cast<const MessageHeader*>(buffer.data() + sizeof(Ethernet::Header));
                       auto ebb = EbbRef<EbbRep>{msg_header->ebb};
                       ebb->HandleMessage(id, buffer +
                                          sizeof(Ethernet::Header) +
                                          sizeof(MessageHeader));
    });
}
