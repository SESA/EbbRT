//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "GlobalIdMap.h"
#include "../CapnpMessage.h"
#include "../Debug.h"
#include "GlobalIdMessage.capnp.h"  //NOLINT

EBBRT_PUBLISH_TYPE(ebbrt, DefaultGlobalIdMap);

void ebbrt::InstallGlobalIdMap() {
  ebbrt::kprintf("Installing default GlobalIdMap\n");
  ebbrt::DefaultGlobalIdMap::Create(ebbrt::kGlobalIdMapId);
}

ebbrt::DefaultGlobalIdMap::DefaultGlobalIdMap()
    : Messagable<DefaultGlobalIdMap>(kGlobalIdMapId) {}

ebbrt::Future<void> ebbrt::DefaultGlobalIdMap::Set(EbbId id, std::string data,
                                                   std::string path) {
  std::lock_guard<std::mutex> lock(m_);
  map_[id] = std::move(data);
  return MakeReadyFuture<void>();
}

ebbrt::Future<std::string> ebbrt::DefaultGlobalIdMap::Get(EbbId id,
                                                          std::string path) {
  return MakeReadyFuture<std::string>(map_[id]);
}

void ebbrt::DefaultGlobalIdMap::ReceiveMessage(Messenger::NetworkId nid,
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
