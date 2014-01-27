//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_

#include <vector>

#include <lwip/netif.h>

#include <ebbrt/Buffer.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/Main.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/Trans.h>

struct tcp_pcb;

namespace ebbrt {
class EthernetDevice {
 public:
  virtual void Send(ConstBufferList l) = 0;
  virtual const std::array<char, 6> &GetMacAddress() = 0;
  virtual ~EthernetDevice() {}
};

class NetworkManager {
 public:
  class Interface {
   public:
    Interface(EthernetDevice &ether_dev, size_t idx);
    void ReceivePacket(MutableBuffer buf, uint32_t len);
    void Send(ConstBufferList l);
    const std::array<char, 6> &MacAddress();

   private:
    EthernetDevice &ether_dev_;
    struct netif netif_;
    size_t idx_;

    friend class NetworkManager;
  };

  class TcpPcb {
   public:
    TcpPcb();
    ~TcpPcb();
    void Bind(uint16_t port);
    void Listen();
    void Accept(std::function<void(TcpPcb)> callback);
    void Connect(struct ip_addr *ipaddr, uint16_t port,
                 std::function<void()> callback = []() {});
    void Receive(std::function<void(MutableBufferList)> callback);

   private:
    explicit TcpPcb(struct tcp_pcb *pcb);

    static err_t Accept_Handler(void *arg, struct tcp_pcb *newpcb, err_t err);
    static err_t Connect_Handler(void *arg, struct tcp_pcb *pcb, err_t err);
    static err_t Receive_Handler(void *arg, struct tcp_pcb *pcb, struct pbuf *p,
                                 err_t err);

    struct tcp_pcb *pcb_;
    std::function<void(TcpPcb)> accept_callback_;
    std::function<void()> connect_callback_;
    std::function<void(MutableBufferList)> receive_callback_;
  };

  static void Init();
  static NetworkManager &HandleFault(EbbId id);
  Interface &NewInterface(EthernetDevice &ether_dev);

 private:
  friend void ebbrt::Main(ebbrt::MultibootInformation *mbi);
  void AcquireIPAddress();

  std::vector<Interface> interfaces_;
};
constexpr auto network_manager = EbbRef<NetworkManager>(kNetworkManagerId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_
