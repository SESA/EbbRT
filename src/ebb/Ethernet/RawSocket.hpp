#ifndef EBBRT_EBB_ETHERNET_RAWSOCKET_HPP
#define EBBRT_EBB_ETHERNET_RAWSOCKET_HPP

#include <unordered_map>

#include <pcap.h>

#include "ebb/Ethernet/Ethernet.hpp"

namespace ebbrt {
  class RawSocket : public Ethernet {
  public:
    static EbbRoot* ConstructRoot();

    RawSocket();
    void Send(BufferList buffers,
              std::function<void()> cb = nullptr) override;
    const uint8_t* MacAddress() override;
    void SendComplete() override;
    void Register(uint16_t ethertype,
                  std::function<void(const uint8_t*, size_t)> func) override;
    void Receive() override;
  private:
    uint8_t mac_addr_[6];
    pcap_t* pdev_;
    std::unordered_map<uint16_t,
                       std::function<void(const uint8_t*, size_t)> > map_;
  };
}

#endif
