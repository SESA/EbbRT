//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/GlobalIdMap.h>

#include <ebbrt/CapnpMessage.h>

#include "GlobalIdMessage.capnp.h"  //NOLINT

EBBRT_PUBLISH_TYPE(ebbrt, GlobalIdMap);

ebbrt::GlobalIdMap::GlobalIdMap() : Messagable<GlobalIdMap>(kGlobalIdMapId) {}

ebbrt::Future<void> ebbrt::GlobalIdMap::Set(EbbId id, std::string data) {
  std::lock_guard<std::mutex> lock(m_);
  map_[id] = std::move(data);
  return MakeReadyFuture<void>();
}

void ebbrt::GlobalIdMap::ReceiveMessage(Messenger::NetworkId nid,
                                        std::unique_ptr<IOBuf>&& buf) {
  auto reader = IOBufMessageReader(std::move(buf));
  auto request = reader.getRoot<global_id_map_message::Request>();

  switch (request.which()) {
  case global_id_map_message::Request::Which::GET_REQUEST: {
    auto get_request = request.getGetRequest();
    IOBufMessageBuilder message;
    auto builder = message.initRoot<global_id_map_message::Reply>();
    auto get_builder = builder.initGetReply();
    get_builder.setMessageId(get_request.getMessageId());
    auto ebbid = get_request.getEbbId();
    {
      std::lock_guard<std::mutex> lock(m_);
      auto& data = map_[ebbid];
      auto reader = capnp::Data::Reader(
          reinterpret_cast<const capnp::byte*>(data.data()), data.size());
      get_builder.setData(reader);
    }

    SendMessage(nid, AppendHeader(message));

    break;
  }
  case global_id_map_message::Request::Which::SET_REQUEST: {
    throw std::runtime_error("UNIMPLEMENTED Set Request");
    break;
  }
  }
}
