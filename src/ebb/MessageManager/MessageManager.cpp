#include <cstdint>
#include <cstdlib>

#include "arch/inet.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/Ethernet/Ethernet.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "lrt/assert.hpp"

namespace {
  constexpr uint16_t MESSAGE_MANAGER_ETHERTYPE = 0x8812;
}

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
  const uint8_t* addr = ethernet->MacAddress();
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
  eth_header->ethertype = htons(MESSAGE_MANAGER_ETHERTYPE);

  MessageHeader* msg_header = reinterpret_cast<MessageHeader*>(eth_header + 1);
  msg_header->ebb = id;

  buffers.emplace_front(eth_header, sizeof(MessageHeader) + sizeof(Ethernet::Header));
  LRT_ASSERT(!cb);
  ethernet->Send(std::move(buffers),
                 [=]() {
                   free(addr);
                 });
}

void
ebbrt::MessageManager::StartListening()
{
  ethernet->Register(MESSAGE_MANAGER_ETHERTYPE,
                     [](const uint8_t* buf, size_t len) {
                       buf += 14; //sizeof ethernet header
                       auto mh = reinterpret_cast<const MessageHeader*>(buf);
                       EbbRef<EbbRep> ebb {mh->ebb};
                       ebb->HandleMessage(buf + sizeof(MessageHeader),
                                          len - sizeof(MessageHeader) -
                                          14);
    });
}
