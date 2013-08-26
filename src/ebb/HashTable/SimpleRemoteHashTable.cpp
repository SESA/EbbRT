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
#include "ebb/HashTable/SimpleRemoteHashTable.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

ebbrt::EbbRoot*
ebbrt::SimpleRemoteHashTable::ConstructRoot()
{
  return new SharedRoot<SimpleRemoteHashTable>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol("RemoteHashTable",
                        ebbrt::SimpleRemoteHashTable::ConstructRoot);
}

ebbrt::SimpleRemoteHashTable::SimpleRemoteHashTable(EbbId id)
  : RemoteHashTable{id}
{
}

ebbrt::Future<ebbrt::Buffer>
ebbrt::SimpleRemoteHashTable::Get(NetworkId from, const std::string& key)
{
  auto buffer = message_manager->Alloc(sizeof(Header) +
                                       key.length());

  auto header = reinterpret_cast<Header*>(buffer.data());
  header->op = GET_REQUEST;
  auto op_id = op_id_.fetch_add(1, std::memory_order_relaxed);
  header->op_id = op_id;

  auto keybuf = buffer.data() + sizeof(Header);
  memcpy(keybuf, key.c_str(), key.length());

  message_manager->Send(from, ebbid_, buffer);

  //Store a promise to be fulfilled later
  auto pair = promise_map_.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(op_id),
                                   std::forward_as_tuple());

  return pair.first->second.GetFuture();
}

void
ebbrt::SimpleRemoteHashTable::Set(const std::string& key, Buffer val)
{
  table_.emplace(key, std::move(val));
}

void
ebbrt::SimpleRemoteHashTable::HandleMessage(NetworkId from,
                                            Buffer buffer)
{
  auto header = reinterpret_cast<const Header*>(buffer.data());
  switch (header->op) {
  case HTOp::GET_REQUEST:
    HandleGetRequest(from, header->op_id, buffer + sizeof(Header));
    break;
  case HTOp::GET_RESPONSE:
    HandleGetResponse(from, header->op_id, buffer + sizeof(Header));
    break;
  }
}

void
ebbrt::SimpleRemoteHashTable::HandleGetRequest(NetworkId from,
                                               unsigned op_id,
                                               Buffer buffer)
{
  auto it = table_.find(std::string(buffer.data(), buffer.length()));

  if (it == table_.end()) {
    //no value, just return the header
    auto buf = message_manager->Alloc(sizeof(Header));
    auto header = reinterpret_cast<Header*>(buf.data());
    header->op = GET_RESPONSE;
    header->op_id = op_id;

    message_manager->Send(from, ebbid_, buf);
  } else {
    auto buf = message_manager->Alloc(sizeof(Header) + it->second.length());
    auto header = reinterpret_cast<Header*>(buf.data());
    header->op = GET_RESPONSE;
    header->op_id = op_id;

    auto val = buf.data() + sizeof(Header);
    memcpy(val, it->second.data(), it->second.length());

    message_manager->Send(from, ebbid_, buf);
  }
}
void
ebbrt::SimpleRemoteHashTable::HandleGetResponse(NetworkId from,
                                                unsigned op_id,
                                                Buffer buffer)
{
  auto it = promise_map_.find(op_id);
  assert(it != promise_map_.end());

  it->second.SetValue(std::move(buffer));
  promise_map_.erase(it);
}
