//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "MsgTst.h"
#include <ebbrt/Debug.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/Future.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/UniqueIOBuf.h>

// This is *IMPORTANT*, it allows the messenger to resolve remote HandleFaults
EBBRT_PUBLISH_TYPE(, MsgTst);

using namespace ebbrt;

// We don't store anything in the GlobalIdMap, just return the EbbRef
EbbRef<MsgTst> MsgTst::Create(EbbId id) { return EbbRef<MsgTst>(id); }

// This Ebb is implemented with one representative per machine
MsgTst& MsgTst::HandleFault(EbbId id) {
  {
    // First we check if the representative is in the LocalIdMap (using a
    // read-lock)
    LocalIdMap::ConstAccessor accessor;
    auto found = local_id_map->Find(accessor, id);
    if (found) {
      auto& rep = *boost::any_cast<MsgTst*>(accessor->second);
      EbbRef<MsgTst>::CacheRef(id, rep);
      return rep;
    }
  }

  MsgTst* rep;
  {
    // Try to insert an entry into the LocalIdMap while holding an exclusive
    // (write) lock
    LocalIdMap::Accessor accessor;
    auto created = local_id_map->Insert(accessor, id);
    if (unlikely(!created)) {
      // We raced with another writer, use the rep it created and return
      rep = boost::any_cast<MsgTst*>(accessor->second);
    } else {
      // Create a new rep and insert it into the LocalIdMap
      rep = new MsgTst(id);
      accessor->second = rep;
    }
  }
  // Cache the reference to the rep in the local translation table
  EbbRef<MsgTst>::CacheRef(id, *rep);
  return *rep;
}

MsgTst::MsgTst(EbbId ebbid) : Messagable<MsgTst>(ebbid) {}

std::unique_ptr<MutIOBuf> MsgTst::RandomMsg(size_t bytes) {
  std::random_device rd;
  std::default_random_engine rng(rd());
  std::uniform_int_distribution<> dist(0, sizeof(alphanum) / sizeof(*alphanum) -
                                              2);
  std::string str;
  str.reserve(bytes);
  std::generate_n(std::back_inserter(str), bytes,
                  [&]() { return alphanum[dist(rng)]; });

  auto buf = MakeUniqueIOBuf(bytes);
  auto dp = buf->GetMutDataPointer();
  auto i = buf->Length();
  while (i--) {
    auto ptr = dp.Data() + i;
    *ptr = alphanum[dist(rng)];
  };
  return std::move(buf);
}

std::vector<ebbrt::Future<uint32_t>>
MsgTst::SendMessages(Messenger::NetworkId nid, uint32_t count, size_t size) {
  uint32_t id;
  assert(size >= sizeof(uint32_t));
  assert(count <= std::numeric_limits<uint32_t>::max());

  std::vector<ebbrt::Future<uint32_t>> ret;
  {
    std::lock_guard<std::mutex> guard(m_);
    id = id_;  // Get a new id (always even)
    id_ += 2 * count;
  }
  while (count--) {
    Promise<uint32_t> promise;
    bool inserted;
    auto f = promise.GetFuture();
    ret.push_back(std::move(f));
    std::tie(std::ignore, inserted) =
        promise_map_.emplace(id, std::move(promise));
    assert(inserted);
    auto buf = RandomMsg(size);
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id + 1;  // Ping messages are odd
    id += 2;
    SendMessage(nid, std::move(buf)).Block();
  }
  return ret;
}

void MsgTst::ReceiveMessage(Messenger::NetworkId nid,
                            std::unique_ptr<IOBuf>&& iobuf) {
  auto dp = iobuf->GetDataPointer();
  auto id = dp.Get<uint32_t>();
  // Ping messages use id + 1, so they are always odd
  if (id & 1) {
    // Received Ping
    auto buf = MakeUniqueIOBuf(sizeof(uint32_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint32_t>() = id - 1;  // Send back with the original id
    SendMessage(nid, std::move(buf));
  } else {
    // Received Pong, lookup in the hash table for our promise and set it
    std::lock_guard<std::mutex> guard(m_);
    auto it = promise_map_.find(id);
    assert(it != promise_map_.end());
    it->second.SetValue(id);
    promise_map_.erase(it);
  }
}
