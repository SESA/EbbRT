//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_MESSAGE_H_
#define COMMON_SRC_INCLUDE_EBBRT_MESSAGE_H_

#include <ebbrt/Messenger.h>

namespace ebbrt {
class MessagableBase {
 public:
  virtual ~MessagableBase() {}
  virtual void ReceiveMessageInternal(Messenger::NetworkId nid, Buffer buf) = 0;
};

struct Hasher {
  std::size_t operator()(uint64_t val) const { return val; }
};
extern std::unordered_map<uint64_t, MessagableBase& (*)(EbbId), Hasher>
fault_map;

template <typename T> class PublishMessagable {
 public:
  PublishMessagable() {
    fault_map.emplace(typeid(T).hash_code(), T::HandleFault);
  }
};

template <typename T> class Messagable : public MessagableBase {
 public:
  explicit Messagable(EbbId id) : id_(id) {}
  Future<void> SendMessage(Messenger::NetworkId nid,
                           std::shared_ptr<const Buffer> buf) {
    return messenger->Send(nid, id_, typeid(T).hash_code(), std::move(buf));
  }
  void ReceiveMessageInternal(Messenger::NetworkId nid, Buffer buf) override {
    static_cast<T*>(this)->ReceiveMessage(nid, std::move(buf));
  }

 private:
  static PublishMessagable<T> publish;

  EbbId id_;
};

MessagableBase& GetMessagableRef(EbbId id, uint64_t type_code);
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_MESSAGE_H_
