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
  private:
    uint8_t mac_addr_[6];
  };
  extern char message_manager_id_resv
  __attribute__ ((section ("static_ebb_ids")));
  extern "C" char static_ebb_ids_start[];
  const EbbRef<MessageManager> message_manager =
    EbbRef<MessageManager>(static_cast<EbbId>(&message_manager_id_resv -
                                              static_ebb_ids_start));
}
#endif
