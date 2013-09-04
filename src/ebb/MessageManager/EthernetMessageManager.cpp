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
  return (ethernet->Alloc(size + sizeof(MessageHeader)) +
          sizeof(MessageHeader));
}

void
ebbrt::EthernetMessageManager::Send(NetworkId to,
                                    EbbId id,
                                    Buffer buffer)
{
  auto buf = buffer - sizeof(MessageHeader);
  auto header = reinterpret_cast<MessageHeader*>(buf.data());
  header->ebb = id;

  ethernet->Send(std::move(buf), to.mac_addr, mac_addr_,
                 htons(MESSAGE_MANAGER_ETHERTYPE));

//   void* addr = std::malloc(sizeof(MessageHeader) + sizeof(Ethernet::Header));

//   Ethernet::Header* eth_header = static_cast<Ethernet::Header*>(addr);
//   std::copy(&to.mac_addr[0], &to.mac_addr[6], eth_header->destination);
//   std::copy(&mac_addr_[0], &mac_addr_[6], eth_header->source);
//   eth_header->ethertype = htons(MESSAGE_MANAGER_ETHERTYPE);

//   MessageHeader* msg_header = reinterpret_cast<MessageHeader*>(eth_header + 1);
//   msg_header->ebb = id;

//   buffers.emplace_front(eth_header, sizeof(MessageHeader) + sizeof(Ethernet::Header));
// #ifdef __linux__
//   assert(!cb);
// #elif __ebbrt__
//   LRT_ASSERT(!cb);
// #endif
//   ethernet->Send(std::move(buffers),
//                  [=]() {
//                    free(addr);
//                  });
}

void
ebbrt::EthernetMessageManager::StartListening()
{
  ethernet->Register(MESSAGE_MANAGER_ETHERTYPE,
                     [](Buffer buffer, const char from[6]) {
                       auto mh = reinterpret_cast<const MessageHeader*>(buffer.data());
                       auto ebb = EbbRef<EbbRep>{mh->ebb};
                       NetworkId id;
                       std::memcpy(id.mac_addr, from, 6);
                       ebb->HandleMessage(id, buffer + sizeof(MessageHeader));
    });
}
