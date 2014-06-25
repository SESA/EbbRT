// //          Copyright Boston University SESA Group 2013 - 2014.
// // Distributed under the Boost Software License, Version 1.0.
// //    (See accompanying file LICENSE_1_0.txt or copy at
// //          http://www.boost.org/LICENSE_1_0.txt)
// #include <ebbrt/GlobalIdMap.h>

// #include <ebbrt/Align.h>
// #include <ebbrt/CapnpMessage.h>
// #include <ebbrt/Messenger.h>

// #include "GlobalIdMessage.capnp.h"

// EBBRT_PUBLISH_TYPE(ebbrt, GlobalIdMap);

// namespace {
// uint32_t frontend_ip;
// }

// ebbrt::GlobalIdMap::GlobalIdMap()
//     : Messagable<GlobalIdMap>(kGlobalIdMapId), val_(0) {}

// void ebbrt::GlobalIdMap::SetAddress(uint32_t addr) { frontend_ip = addr; }

// namespace {

// struct Header {
//   uint32_t num_segments;
//   uint32_t segment_sizes[0];
// };
// }

// ebbrt::Future<std::string> ebbrt::GlobalIdMap::Get(EbbId id) {
//   lock_.lock();
//   auto v = val_++;
//   auto& p = map_[v];
//   lock_.unlock();

//   IOBufMessageBuilder message;
//   auto builder = message.initRoot<global_id_map_message::Request>();
//   auto get_builder = builder.initGetRequest();
//   get_builder.setMessageId(v);
//   get_builder.setEbbId(id);

//   SendMessage(Messenger::NetworkId(frontend_ip), AppendHeader(message));

//   return p.GetFuture();
// }

// void ebbrt::GlobalIdMap::ReceiveMessage(Messenger::NetworkId nid,
//                                         std::unique_ptr<IOBuf>&& b) {
//   auto reader = IOBufMessageReader(std::move(b));
//   auto reply = reader.getRoot<global_id_map_message::Reply>();

//   switch (reply.which()) {
//   case global_id_map_message::Reply::Which::GET_REPLY: {
//     auto get_reply = reply.getGetReply();
//     auto mid = get_reply.getMessageId();
//     auto it = map_.find(mid);
//     kassert(it != map_.end());
//     auto data = get_reply.getData();
//     it->second.SetValue(
//         std::string(reinterpret_cast<const char*>(data.begin()),
// data.size()));
//     break;
//   }
//   case global_id_map_message::Reply::Which::SET_REPLY: {
//     throw std::runtime_error("UNIMPLEMENTED Set Reply");
//     break;
//   }
//   }
// }
