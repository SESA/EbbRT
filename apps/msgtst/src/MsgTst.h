//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef APPS_MSGTST_SRC_MSGTST_H_
#define APPS_MSGTST_SRC_MSGTST_H_

#include <unordered_map>
#include <iostream>
#include <iterator>
#include <vector>
#include <random>
#include <algorithm>

#include <ebbrt/EbbAllocator.h>
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/IOBuf.h>
#include <ebbrt/Future.h>
#include <ebbrt/Message.h>

/* The MsgTst Ebb allows both hosted and native implementations to send "PING"
 * messages via the messenger to another node (machine). The remote node
 * responds back and the initiator fulfills its Promise. */

static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

class MsgTst : public ebbrt::Messagable<MsgTst> {
 public:
  static ebbrt::EbbRef<MsgTst>
  Create(ebbrt::EbbId id = ebbrt::ebb_allocator->Allocate());

  MsgTst(ebbrt::EbbId ebbid);
  std::unique_ptr<ebbrt::MutIOBuf> RandomMsg(size_t bytes);
  static MsgTst& HandleFault(ebbrt::EbbId id);
  std::vector<ebbrt::Future<uint32_t>> SendMessages(ebbrt::Messenger::NetworkId nid, uint32_t count, size_t size);
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                      std::unique_ptr<ebbrt::IOBuf>&& buffer);

 private:
  std::mutex m_;
  std::unordered_map<uint32_t, ebbrt::Promise<uint32_t>> promise_map_;
  uint32_t id_{0};
};

#endif  // APPS_MSGTST_SRC_MSGTST_H_
