//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_

#include <map>
#include <vector>

#include <lwip/netif.h>

#include <ebbrt/Buffer.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/Future.h>
#include <ebbrt/Main.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/Trans.h>

struct tcp_pcb;

namespace ebbrt {
class EthernetDevice {
 public:
  virtual void Send(std::shared_ptr<const Buffer> l) = 0;
  virtual const std::array<char, 6>& GetMacAddress() = 0;
  virtual ~EthernetDevice() {}
};

class NetworkManager {
 public:
  class Interface {
   public:
    Interface(EthernetDevice& ether_dev, size_t idx);
    void Receive(Buffer buf);
    void Send(std::shared_ptr<const Buffer> buf);
    const std::array<char, 6>& MacAddress();

   private:
    EthernetDevice& ether_dev_;
    struct netif netif_;
    size_t idx_;

    friend class NetworkManager;
  };

  class TcpPcb {
   public:
    TcpPcb();
    TcpPcb(TcpPcb&&);

    TcpPcb& operator=(TcpPcb&&);
    ip_addr_t GetRemoteAddress() const;
    void Bind(uint16_t port);
    void Listen();
    void Accept(std::function<void(TcpPcb)> callback);
    Future<void> Connect(struct ip_addr* ipaddr, uint16_t port);
    void Receive(std::function<void(TcpPcb&, Buffer)> callback);
    Future<void> Send(std::shared_ptr<const Buffer> data);

   private:
    static void Deleter(struct tcp_pcb* object);
    explicit TcpPcb(struct tcp_pcb* pcb);

    static err_t Accept_Handler(void* arg, struct tcp_pcb* newpcb, err_t err);
    static err_t Connect_Handler(void* arg, struct tcp_pcb* pcb, err_t err);
    static err_t Receive_Handler(void* arg, struct tcp_pcb* pcb, struct pbuf* p,
                                 err_t err);
    static err_t SentHandler(void* arg, struct tcp_pcb* pcb, uint16_t len);

    std::unique_ptr<struct tcp_pcb, void (*)(struct tcp_pcb*)> pcb_;
    std::function<void(TcpPcb)> accept_callback_;
    Promise<void> connect_promise_;
    std::function<void(TcpPcb&, Buffer)> receive_callback_;
    uint64_t sent_;
    uint64_t next_;
    std::map<uint64_t, Promise<void>> ack_map_;
  };

  static void Init();
  static NetworkManager& HandleFault(EbbId id);
  Interface& NewInterface(EthernetDevice& ether_dev);

 private:
  friend void ebbrt::Main(ebbrt::multiboot::Information* mbi);
  void AcquireIPAddress();

  std::vector<Interface> interfaces_;
};
constexpr auto network_manager = EbbRef<NetworkManager>(kNetworkManagerId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_
