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

#include <cstdlib>

#include "mpi.h"

#include "app/app.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/HashTable/DistributedHashTable.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

//#define TRACE 1

#ifdef TRACE
struct TracePoint {
  bool request;
  uint32_t who;
  char* key;
  size_t key_size;
  uint32_t val;
};

size_t num_tracepoints;
TracePoint* tracepoints;
std::vector<TracePoint> tracepoints_vec;
#endif

ebbrt::EbbRoot*
ebbrt::DistributedHashTable::ConstructRoot()
{
  return new SharedRoot<DistributedHashTable>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol("HashTable",
                        ebbrt::DistributedHashTable::ConstructRoot);
}

ebbrt::DistributedHashTable::DistributedHashTable(EbbId id) :
  HashTable{id}
{
  //FIXME: MPI specific
  if (MPI_Comm_rank(MPI_COMM_WORLD, &rank_) != MPI_SUCCESS) {
    throw std::runtime_error("MPI_Comm_rank failed");
  }

  int size;
  if (MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS) {
    throw std::runtime_error("MPI_Comm_size failed");
  }
  node_count_ = size;
}

ebbrt::Future<std::pair<char*, size_t> >
ebbrt::DistributedHashTable::Get(const char* key, size_t key_size,
                                 std::function<void()> sent)
{
  //Find the node responsible for this key
  std::hash<std::string> hash_func;
  //FIXME: MPI specific
  auto hash = hash_func(std::string(key, key_size));
  NetworkId to;
  to.rank = hash % node_count_;

  auto header = new GetRequest;
  header->op = GET_REQUEST;
  auto op_id = op_id_.fetch_add(1, std::memory_order_relaxed);
  header->op_id = op_id;

  BufferList list = {
    std::pair<const void*, size_t>(header, sizeof(GetRequest)),
    std::pair<const void*, size_t>(key, key_size)
  };

  message_manager->Send(to, ebbid_, std::move(list),
                        [=]() {
                          delete header;
                          if (sent) {
                            sent();
                          }
                        });

  //Store a promise to be fulfilled later
  auto pair = promise_map_.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(op_id),
                                   std::forward_as_tuple());

  return pair.first->second.GetFuture();
}

void
ebbrt::DistributedHashTable::Set(const char* key, size_t key_size,
                                 const char* val, size_t val_size,
                                 std::function<void()> sent)
{
  //Find the node responsible for this key
  std::hash<std::string> hash_func;
  //FIXME: MPI specific
  auto hash = hash_func(std::string(key, key_size));
  NetworkId to;
  to.rank = hash % node_count_;

  auto header = new SetRequest;
  header->op = SET_REQUEST;
  header->key_size = key_size;

  BufferList list = {
    std::pair<const void*, size_t>(header, sizeof(SetRequest)),
    std::pair<const void*, size_t>(key, key_size),
    std::pair<const void*, size_t>(val, val_size)
  };

  message_manager->Send(to, ebbid_, std::move(list),
                        [=]() {
                          delete header;
                          if (sent) {
                            sent();
                          }
                        });
}

ebbrt::Future<std::pair<char*, size_t> >
ebbrt::DistributedHashTable::SyncGet(const char* key, size_t key_size,
                                     uint32_t waitfor,
                                     std::function<void()> sent)
{
  //Find the node responsible for this key
  std::hash<std::string> hash_func;
  //FIXME: MPI specific
  auto hash = hash_func(std::string(key, key_size));
  NetworkId to;
  to.rank = hash % node_count_;

  //Create message header
  auto header = new SyncGetRequest;
  header->op = SYNC_GET_REQUEST;
  auto op_id = op_id_.fetch_add(1, std::memory_order_relaxed);
  header->op_id = op_id;
  header->waitfor = waitfor;

  BufferList list = {
    std::pair<const void*, size_t>(header , sizeof(SyncGetRequest)),
    std::pair<const void*, size_t>(key, key_size)
  };

  message_manager->Send(to, ebbid_, std::move(list),
                        [=]() {
                          delete header;
                          if (sent) {
                            sent();
                          }
                        });

  //Store a promise to be fulfilled later
  auto pair = promise_map_.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(op_id),
                                   std::forward_as_tuple());

  return pair.first->second.GetFuture();
}

void
ebbrt::DistributedHashTable::SyncSet(const char* key, size_t key_size,
                                     const char* val, size_t val_size,
                                     uint32_t delta,
                                     std::function<void()> sent)
{
  //Find the node responsible for this key
  std::hash<std::string> hash_func;
  //FIXME: MPI specific
  auto hash = hash_func(std::string(key, key_size));
  NetworkId to;
  to.rank = hash % node_count_;

  //Create message header
  auto header = new SyncSetRequest;
  header->op = SYNC_SET_REQUEST;
  header->key_size = key_size;
  header->delta = delta;

  BufferList list = {
    std::pair<const void*, size_t>(header, sizeof(SyncSetRequest)),
    std::pair<const void*, size_t>(key, key_size),
    std::pair<const void*, size_t>(val, val_size)
  };

  message_manager->Send(to, ebbid_, std::move(list),
                        [=]() {
                          delete header;
                          if (sent) {
                            sent();
                          }
                        });
}

ebbrt::Future<uint32_t>
ebbrt::DistributedHashTable::Increment(const char* key,
                                       size_t key_size,
                                       std::function<void()> sent)
{
  //Find the node responsible for this key
  std::hash<std::string> hash_func;
  //FIXME: MPI specific
  auto hash = hash_func(std::string(key, key_size));
  NetworkId to;
  to.rank = hash % node_count_;

  //Create message header
  auto header = new IncrementRequest;
  header->op = INCREMENT_REQUEST;
  auto op_id = op_id_.fetch_add(1, std::memory_order_relaxed);
  header->op_id = op_id;

  BufferList list = {
    std::pair<const void*, size_t>(header, sizeof(IncrementRequest)),
    std::pair<const void*, size_t>(key, key_size)
  };

#ifdef TRACE
  TracePoint tp;
  tp.request = true;
  tp.who = to.rank;
  tp.key = static_cast<char*>(malloc(key_size));
  memcpy(tp.key, key, key_size);
  tp.key_size = key_size;
  tp.val = 0;
  tracepoints_vec.push_back(tp);
  tracepoints = tracepoints_vec.data();
  num_tracepoints++;
#endif

  message_manager->Send(to, ebbid_, std::move(list),
                        [=]() {
                          delete header;
                          if (sent) {
                            sent();
                          }
                        });

  //Store a promise to be fulfilled later
  auto pair = increment_promise_map_.emplace(std::piecewise_construct,
                                             std::forward_as_tuple(op_id),
                                             std::forward_as_tuple());

  return pair.first->second.GetFuture();
}

void
ebbrt::DistributedHashTable::HandleMessage(NetworkId from, const char* buf,
                                           size_t len)
{
  assert(len >= sizeof(DHTOp));

  auto op = reinterpret_cast<const DHTOp*>(buf);
  switch(*op) {
  case DHTOp::GET_REQUEST:
    assert(len >= sizeof(GetRequest));
    HandleGetRequest(from, *reinterpret_cast<const GetRequest*>(buf),
                     buf + sizeof(GetRequest), len - sizeof(GetRequest));
    break;
  case DHTOp::GET_RESPONSE:
    assert(len >= sizeof(GetResponse));
    HandleGetResponse(*reinterpret_cast<const GetResponse*>(buf),
                      buf + sizeof(GetResponse), len - sizeof(GetResponse));
    break;
  case DHTOp::SET_REQUEST:
    assert(len >= sizeof(SetRequest));
    HandleSetRequest(*reinterpret_cast<const SetRequest*>(buf),
                     buf + sizeof(SetRequest), len - sizeof(SetRequest));
    break;
  case DHTOp::SYNC_GET_REQUEST:
    assert(len >= sizeof(SyncGetRequest));
    HandleSyncGetRequest(from, *reinterpret_cast<const SyncGetRequest*>(buf),
                         buf + sizeof(SyncGetRequest),
                         len - sizeof(SyncGetRequest));
    break;
  case DHTOp::SYNC_SET_REQUEST:
    assert(len >= sizeof(SyncSetRequest));
    HandleSyncSetRequest(*reinterpret_cast<const SyncSetRequest*>(buf),
                         buf + sizeof(SyncSetRequest),
                         len - sizeof(SyncSetRequest));
    break;
  case DHTOp::INCREMENT_REQUEST:
    assert(len >= sizeof(IncrementRequest));
    HandleIncrementRequest(from, *reinterpret_cast<const IncrementRequest*>(buf),
                           buf + sizeof(IncrementRequest),
                           len - sizeof(IncrementRequest));
    break;
  case DHTOp::INCREMENT_RESPONSE:
    assert(len >= sizeof(IncrementResponse));
    HandleIncrementResponse(*reinterpret_cast<const IncrementResponse*>(buf));
    break;
  }
}

void
ebbrt::DistributedHashTable::GetRespond(NetworkId to,
                                        unsigned op_id,
                                        const std::string& key)
{
  auto it = table_.find(key);

  //create header
  auto header = new GetResponse;
  header->op = GET_RESPONSE;
  header->op_id = op_id;

  if (it == table_.end()) {
    //no value, just return the header
    BufferList list = {
      std::pair<const void*, size_t>(header, sizeof(GetResponse))
    };
    message_manager->Send(to, ebbid_, std::move(list),
                          [=]() {
                            delete header;
                          });
  } else {
    //allocate and copy the value
    auto val = new std::string(it->second);
    BufferList list = {
      std::pair<const void*, size_t>(header, sizeof(GetResponse)),
      std::pair<const void*, size_t>(val->c_str(), val->length())
    };
    message_manager->Send(to, ebbid_, std::move(list),
                          [=]() {
                            delete val;
                            delete header;
                          });
  }
}

void
ebbrt::DistributedHashTable::HandleGetRequest(NetworkId from,
                                              const GetRequest& req,
                                              const char* key, size_t len)
{
  std::string keystr{key, len};
  GetRespond(from, req.op_id, keystr);
}

void
ebbrt::DistributedHashTable::HandleGetResponse(const GetResponse& resp,
                                               const char* val, size_t len)
{
  //Get the promise associated with the original request
  auto it = promise_map_.find(resp.op_id);
  assert(it != promise_map_.end());

  char *buf = static_cast<char*>(malloc(len));
  std::memcpy(buf, val, len);
  it->second.Set(std::make_pair(buf, len));
  promise_map_.erase(it);
}

void
ebbrt::DistributedHashTable::HandleSetRequest(const SetRequest& req,
                                              const char* buf, size_t len)
{
  table_[std::string(buf, req.key_size)] =
    std::string(buf + req.key_size, len - req.key_size);
}

void
ebbrt::DistributedHashTable::HandleSyncGetRequest(NetworkId from,
                                                  const SyncGetRequest& req,
                                                  const char* key, size_t len)
{
  std::string keystr{key, len};
  auto& sync_ent = sync_table_[keystr];

  if (sync_ent.first < req.waitfor) {
    //cannot response right away, queue this request
    sync_ent.second.insert(std::make_pair(req.waitfor,
                                          std::make_pair(from, req.op_id)));
  } else {
    //respond to request
    GetRespond(from, req.op_id, keystr);
  }
}

void
ebbrt::DistributedHashTable::HandleSyncSetRequest(const SyncSetRequest& req,
                                                  const char* buf,
                                                  size_t len)
{
  std::string key{buf, req.key_size};
  //set the string value
  auto& ent = table_[key];
  ent = std::string(buf + req.key_size, len - req.key_size);

  auto& sync_ent = sync_table_[key];
  //increment the count value
  sync_ent.first += req.delta;

  //iterate over the waiting entries that can now be responded to
  for (auto it = sync_ent.second.begin();
       it != sync_ent.second.upper_bound(sync_ent.first);
       ++it) {
    //send response
    auto header = new GetResponse;
    header->op = GET_RESPONSE;
    header->op_id = it->second.second;

    //allocate and copy value
    auto val = new std::string(ent);
    BufferList list = {
      std::pair<const void*, size_t>(header, sizeof(GetResponse)),
      std::pair<const void*, size_t>(val->c_str(), val->length())
    };
    NetworkId to = it->second.first;

    message_manager->Send(to, ebbid_, std::move(list),
                          [=]() {
                            delete val;
                            delete header;
                          });
  }
  //remove responded to requests
  sync_ent.second.erase(sync_ent.second.begin(),
                        sync_ent.second.upper_bound(sync_ent.first));
}

void
ebbrt::DistributedHashTable::HandleIncrementRequest(NetworkId from,
                                                    const IncrementRequest& req,
                                                    const char* key, size_t len)
{
  auto val = value_table_[std::string(key, len)]++;

#ifdef TRACE
  TracePoint tp;
  tp.request = false;
  tp.who = from.rank;
  tp.key = static_cast<char*>(malloc(len));
  memcpy(tp.key, key, len);
  tp.key_size = len;
  tp.val = val;
  tracepoints_vec.push_back(tp);
  tracepoints = tracepoints_vec.data();
  num_tracepoints++;
#endif

  auto header = new IncrementResponse;
  header->op = INCREMENT_RESPONSE;
  header->op_id = req.op_id;
  header->val = val;

  BufferList list = {
    std::pair<const void*, size_t>(header, sizeof(IncrementResponse))
  };

  message_manager->Send(from, ebbid_, std::move(list),
                        [=]() {
                          delete header;
                        });
}

void
ebbrt::DistributedHashTable::HandleIncrementResponse(const IncrementResponse& resp)
{
  auto it = increment_promise_map_.find(resp.op_id);
  assert(it != increment_promise_map_.end());
  it->second.Set(resp.val);
  increment_promise_map_.erase(it);
}
