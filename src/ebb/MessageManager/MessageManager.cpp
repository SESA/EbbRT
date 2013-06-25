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
#include <cstring>

#include "arch/inet.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/Ethernet/Ethernet.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

#ifdef __ebbrt__
#include "lrt/bare/assert.hpp"
#endif

namespace {
  constexpr uint16_t MESSAGE_MANAGER_ETHERTYPE = 0x8812;
}

ebbrt::EbbRoot*
ebbrt::MessageManager::ConstructRoot()
{
  return new SharedRoot<MessageManager>;
}

namespace {
  class MessageHeader {
  public:
    ebbrt::EbbId ebb;
  };
};

ebbrt::MessageManager::MessageManager()
{
  const uint8_t* addr = ethernet->MacAddress();
  std::copy(&addr[0], &addr[6], mac_addr_);
}

void
ebbrt::MessageManager::Send(NetworkId to,
                            EbbId id,
                            BufferList buffers,
                            std::function<void()> cb)
{
  void* addr = std::malloc(sizeof(MessageHeader) + sizeof(Ethernet::Header));

  Ethernet::Header* eth_header = static_cast<Ethernet::Header*>(addr);
  std::copy(&to.mac_addr[0], &to.mac_addr[6], eth_header->destination);
  std::copy(&mac_addr_[0], &mac_addr_[6], eth_header->source);
  eth_header->ethertype = htons(MESSAGE_MANAGER_ETHERTYPE);

  MessageHeader* msg_header = reinterpret_cast<MessageHeader*>(eth_header + 1);
  msg_header->ebb = id;

  buffers.emplace_front(eth_header,
                        sizeof(MessageHeader) + sizeof(Ethernet::Header));
  ethernet->Send(std::move(buffers),
                 [=]() {
                   free(addr);
                   if (cb) {
                     cb();
                   }
                 });
}

void
ebbrt::MessageManager::StartListening()
{
  ethernet->Register(MESSAGE_MANAGER_ETHERTYPE,
                     [](const char* buf, size_t len) {
                       buf += 6; //points to mac source
                       NetworkId id;
                       std::memcpy(id.mac_addr, buf, 6);
                       buf += 8; //points to end of eth header
                       auto mh = reinterpret_cast<const MessageHeader*>(buf);
                       EbbRef<EbbRep> ebb {mh->ebb};
                       ebb->HandleMessage(id,
                                          buf + sizeof(MessageHeader),
                                          len - sizeof(MessageHeader) -
                                          14);
    });
}
