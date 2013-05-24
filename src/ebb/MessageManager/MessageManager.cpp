#include "ebb/SharedRoot.hpp"
#include "ebb/Ethernet/Ethernet.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "lrt/assert.hpp"

char ebbrt::message_manager_id_resv
__attribute__ ((section ("static_ebb_ids")));

ebbrt::EbbRoot*
ebbrt::MessageManager::ConstructRoot()
{
  return new SharedRoot<MessageManager>;
}

namespace {
  class MessageHeader {
  public:
    ebbrt::EbbId ebb;
    ebbrt::NodeId from;
  } __attribute__((packed));
};

void
ebbrt::MessageManager::Send(NodeId node,
                            EbbId id,
                            BufferList buffers,
                            const std::function<void(BufferList)>& cb)
{
  MessageHeader* header = new MessageHeader;
  header->ebb = id;
  header->from = node;
  buffers.emplace_front(header, sizeof(MessageHeader));
  char addr[] = {static_cast<char>(0xff),
                 static_cast<char>(0xff),
                 static_cast<char>(0xff),
                 static_cast<char>(0xff),
                 static_cast<char>(0xff),
                 static_cast<char>(0xff)};
  LRT_ASSERT(!cb);
  ethernet->Send(addr,
                 0x8812,
                 std::move(buffers),
                 [](BufferList list) {
                   auto header =
                     static_cast<const MessageHeader*>(list.front().first);
                   delete header;
                   list.pop_front();
                 });
}
