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

#include "app/app.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/MessageManager/UDPMessageManager.hpp"
#include "ebb/Network/Network.hpp"

namespace {
constexpr uint16_t MESSAGE_MANAGER_PORT = 43200;
}

ebbrt::EbbRoot *ebbrt::UDPMessageManager::ConstructRoot() {
  return new SharedRoot<UDPMessageManager>;
}

// registers symbol for configuration
__attribute__((constructor(65535))) static void _reg_symbol() {
  ebbrt::app::AddSymbol("MessageManager",
                        ebbrt::UDPMessageManager::ConstructRoot);
}

namespace {
class MessageHeader {
public:
  ebbrt::EbbId ebb;
};
}

ebbrt::UDPMessageManager::UDPMessageManager(EbbId id) : MessageManager{ id } {}

ebbrt::Buffer ebbrt::UDPMessageManager::Alloc(size_t size) {
  auto mem = std::malloc(size + sizeof(MessageHeader));
  if (mem == nullptr) {
    throw std::bad_alloc();
  }
  return (Buffer(mem, size + sizeof(MessageHeader)) + sizeof(MessageHeader));
}

void ebbrt::UDPMessageManager::Send(NetworkId to, EbbId id, Buffer buffer) {
  auto buf = buffer - sizeof(MessageHeader);
  auto msg_header = reinterpret_cast<MessageHeader*>(buf.data());
  msg_header->ebb = id;

  network->SendUDP(std::move(buf), to, MESSAGE_MANAGER_PORT);
}

void ebbrt::UDPMessageManager::StartListening() {
  network->RegisterUDP(MESSAGE_MANAGER_PORT, [](Buffer buffer, NetworkId from) {
    auto msg_header = reinterpret_cast<const MessageHeader *>(buffer.data());
    auto ebb = EbbRef<EbbRep>{ msg_header->ebb };
    ebb->HandleMessage(from, buffer + sizeof(MessageHeader));
  });
}
