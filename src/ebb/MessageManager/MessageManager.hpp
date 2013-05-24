#ifndef EBBRT_EBB_MESSAGEMANAGER_MESSAGEMANAGER_HPP
#define EBBRT_EBB_MESSAGEMANAGER_MESSAGEMANAGER_HPP

#include <functional>

#include "misc/buffer.hpp"

namespace ebbrt {
  class MessageManager : public EbbRep {
  public:
    static EbbRoot* ConstructRoot();
    virtual void Send(NodeId node,
                      EbbId ebb,
                      BufferList buffers,
                      const std::function<void(BufferList)>& cb = nullptr);
  };
  extern char message_manager_id_resv
  __attribute__ ((section ("static_ebb_ids")));
  extern "C" char static_ebb_ids_start[];
  const EbbRef<MessageManager> message_manager =
    EbbRef<MessageManager>(static_cast<EbbId>(&message_manager_id_resv -
                                              static_ebb_ids_start));
}
#endif
