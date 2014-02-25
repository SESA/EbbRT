//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/GlobalIdMap.h>

#include <ebbrt/Align.h>
#include <ebbrt/CapnpMessage.h>
#include <ebbrt/Messenger.h>

#include <ebbrt/GlobalIdMessage.capnp.h>

EBBRT_PUBLISH_TYPE(ebbrt, GlobalIdMap);

namespace {
uint32_t frontend_ip;
}

ebbrt::GlobalIdMap::GlobalIdMap()
    : Messagable<GlobalIdMap>(kGlobalIdMapId), val_(0) {}

void ebbrt::GlobalIdMap::SetAddress(uint32_t addr) { frontend_ip = addr; }

namespace {

struct Header {
  uint32_t num_segments;
  uint32_t segment_sizes[0];
};
}

ebbrt::Future<std::string> ebbrt::GlobalIdMap::Get(EbbId id) {
  lock_.lock();
  auto v = val_++;
  auto& p = map_[v];
  lock_.unlock();

  BufferMessageBuilder message;
  auto builder = message.initRoot<global_id_map_message::Request>();
  auto get_builder = builder.initGetRequest();
  get_builder.setMessageId(v);
  get_builder.setEbbId(id);

  auto buf = message.GetBuffer();

  auto num_segments = buf.length();
  kassert(num_segments > 0);
  auto header_size = align::Up(4 + 4 * num_segments, 8);
  auto header_buf = Buffer(header_size);
  auto h = static_cast<Header*>(header_buf.data());
  h->num_segments = num_segments;
  auto it = buf.begin();
  for (unsigned i = 0; i < buf.length(); ++i, ++it) {
    auto seg_size = it->size();
    kassert(seg_size % sizeof(capnp::word) == 0);
    h->segment_sizes[i] = seg_size / sizeof(capnp::word);
  }
  header_buf.emplace_back(std::move(buf));

  SendMessage(Messenger::NetworkId(frontend_ip),
              std::make_shared<const Buffer>(std::move(header_buf)));

  return p.GetFuture();
}

void ebbrt::GlobalIdMap::ReceiveMessage(Messenger::NetworkId nid, Buffer b) {
  auto reader = BufferMessageReader(std::move(b));
  auto reply = reader.getRoot<global_id_map_message::Reply>();

  switch (reply.which()) {
  case global_id_map_message::Reply::Which::GET_REPLY: {
    auto get_reply = reply.getGetReply();
    auto mid = get_reply.getMessageId();
    auto it = map_.find(mid);
    kassert(it != map_.end());
    auto data = get_reply.getData();
    it->second.SetValue(
        std::string(reinterpret_cast<const char*>(data.begin()), data.size()));
    break;
  }
  case global_id_map_message::Reply::Which::SET_REPLY: {
    throw std::runtime_error("UNIMPLEMENTED Set Reply");
    break;
  }
  }
}
