#ifndef EBBRT_EBB_MESSAGEMANAGER_MESSAGEMANAGER_HPP
#define EBBRT_EBB_MESSAGEMANAGER_MESSAGEMANAGER_HPP

#include <functional>

#include "misc/network.hpp"
#include "misc/buffer.hpp"

namespace ebbrt {
  class MessageManager : public EbbRep {
  public:
    static EbbRoot* ConstructRoot();
    MessageManager();
    virtual void Send(NetworkId to,
                      EbbId ebb,
                      BufferList buffers,
                      std::function<void()> cb = nullptr);
    virtual void StartListening();
  private:
    uint8_t mac_addr_[6];
  };
  const EbbRef<MessageManager> message_manager =
    EbbRef<MessageManager>(lrt::trans::find_static_ebb_id("MessageManager"));
}
#endif
