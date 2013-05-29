#include <cstdlib>

#include "arch/inet.hpp"
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
  };
};

ebbrt::MessageManager::MessageManager()
{
  const char* addr = ethernet->MacAddress();
  std::copy(&addr[0], &addr[6], mac_addr_);
}

void
ebbrt::MessageManager::Send(NetworkId to,
                            EbbId id,
                            BufferList buffers,
                            std::function<void()> cb)
{
  void* addr = std::malloc(sizeof(MessageHeader) + sizeof(Ethernet::Header));

  Ethernet::Header* eth_header = static_cast<Ethernet::Header*>(addr);
  std::copy(&to.mac_addr[0], &to.mac_addr[6], eth_header->destination);
  std::copy(&mac_addr_[0], &mac_addr_[6], eth_header->source);
  eth_header->ethertype = htons(0x8812);

  MessageHeader* msg_header = reinterpret_cast<MessageHeader*>(eth_header + 1);
  msg_header->ebb = id;

  buffers.emplace_front(eth_header, sizeof(MessageHeader) + sizeof(Ethernet::Header));
  LRT_ASSERT(!cb);
  ethernet->Send(std::move(buffers),
                 [=]() {
                   free(addr);
                 });
}
