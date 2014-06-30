//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_

#include <ebbrt/AtomicUniquePtr.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/IOBuf.h>
#include <ebbrt/NetEth.h>
#include <ebbrt/NetIp.h>
#include <ebbrt/RcuTable.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {
class EthernetDevice {
 public:
  virtual void Send(std::unique_ptr<IOBuf> buf) = 0;
  virtual const EthernetAddress& GetMacAddress() = 0;
  virtual void Poll() = 0;
  virtual ~EthernetDevice() {}
};

class NetworkManager : public StaticSharedEbb<NetworkManager> {
 public:
  class Interface {
   public:
    struct ItfAddress {
      bool isBroadcast(const Ipv4Address& addr);
      bool isLocalNetwork(const Ipv4Address& addr);

      Ipv4Address address;
      Ipv4Address netmask;
      Ipv4Address gateway;
    };

    struct ItfAddressDeleter {
      void operator()(ItfAddress* p) {
        event_manager->DoRcu([p]() { delete p; });
      }
    };

    explicit Interface(EthernetDevice& ether_dev)
        : address_(nullptr), ether_dev_(ether_dev) {}

    void Receive(std::unique_ptr<MutIOBuf> buf);
    void Send(std::unique_ptr<IOBuf> buf);
    void Poll();
    const EthernetAddress& MacAddress();
    ItfAddress* Address() { return address_.get(); }
    void SetAddress(std::unique_ptr<ItfAddress> address) {
      address_.store(address.release());
    }

   private:
    void ReceiveArp(EthernetHeader& eh, std::unique_ptr<MutIOBuf> buf);
    void ReceiveIp(EthernetHeader& eh, std::unique_ptr<MutIOBuf> buf);
    void ReceiveIcmp(EthernetHeader& eh, Ipv4Header& ih,
                     std::unique_ptr<MutIOBuf> buf);
    void EthArpSend(EthernetHeader& eh, Ipv4Header& ih,
                    std::unique_ptr<MutIOBuf> buf);
    atomic_unique_ptr<ItfAddress, ItfAddressDeleter> address_;
    EthernetDevice& ether_dev_;
  };

  static void Init();

  Interface& NewInterface(EthernetDevice& ether_dev);

 private:
  std::vector<Interface> interfaces_;
  RcuHashTable<ArpEntry, Ipv4Address, &ArpEntry::hook, &ArpEntry::paddr>
  arp_cache_{8};  // 256 buckets

  alignas(cache_size) std::mutex arp_write_lock_;
};

constexpr auto network_manager = EbbRef<NetworkManager>(kNetworkManagerId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_

// #include <map>
// #include <queue>
// #include <vector>

// #include <lwip/netif.h>

// #include <ebbrt/Buffer.h>
// #include <ebbrt/EbbRef.h>
// #include <ebbrt/Future.h>
// #include <ebbrt/Main.h>
// #include <ebbrt/StaticIds.h>
// #include <ebbrt/Trans.h>

// struct tcp_pcb;

// namespace ebbrt {
// class EthernetDevice {
//  public:
//   virtual void Send(std::unique_ptr<IOBuf>&& l) = 0;
//   virtual const std::array<char, 6>& GetMacAddress() = 0;
//   virtual void Poll() = 0;
//   virtual ~EthernetDevice() {}
// };

// class NetworkManager {
//  public:
//   class Interface {
//    public:
//     Interface(EthernetDevice& ether_dev, size_t idx);
//     void Receive(std::unique_ptr<IOBuf>&& buf);
//     void Send(std::unique_ptr<IOBuf>&& buf);
//     void Poll();
//     const std::array<char, 6>& MacAddress();
//     uint32_t IPV4Addr();

//    private:
//     EthernetDevice& ether_dev_;
//     struct netif netif_;
//     size_t idx_;

//     friend class NetworkManager;
//   };

//   class TcpPcb {
//    public:
//     TcpPcb();
//     TcpPcb(TcpPcb&&);

//     TcpPcb& operator=(TcpPcb&&);
//     ip_addr_t GetRemoteAddress() const;
//     uint16_t GetRemotePort() const;
//     void Bind(uint16_t port);
//     void Listen();
//     void ListenWithBacklog(uint8_t backlog);
//     void Accept(std::function<void(TcpPcb)> callback);
//     Future<void> Connect(struct ip_addr* ipaddr, uint16_t port);
//     void
//     Receive(std::function<void(TcpPcb&, std::unique_ptr<IOBuf>&&)> callback);
//     Future<void> Send(std::unique_ptr<IOBuf>&& data);
//     /* TCP congestion control toggles */
//     void EnableNagle();
//     void DisableNagle();
//     bool NagleDisabled();

//    private:
//     static void Deleter(struct tcp_pcb* object);
//     explicit TcpPcb(struct tcp_pcb* pcb);

//     size_t InternalSend(const std::unique_ptr<IOBuf>& data);
//     static err_t Accept_Handler(void* arg, struct tcp_pcb* newpcb, err_t
// err);
//     static err_t Connect_Handler(void* arg, struct tcp_pcb* pcb, err_t err);
//     static err_t Receive_Handler(void* arg, struct tcp_pcb* pcb, struct pbuf*
// p,
//                                  err_t err);
//     static err_t SentHandler(void* arg, struct tcp_pcb* pcb, uint16_t len);

//     std::unique_ptr<struct tcp_pcb, void (*)(struct tcp_pcb*)> pcb_;
//     std::function<void(TcpPcb)> accept_callback_;
//     Promise<void> connect_promise_;
//     std::function<void(TcpPcb&, std::unique_ptr<IOBuf>&&)> receive_callback_;
//     uint64_t sent_;
//     uint64_t next_;
//     std::queue<std::tuple<uint64_t, Promise<void>, std::unique_ptr<IOBuf>>>
//     ack_queue_;
//     std::queue<std::reference_wrapper<std::unique_ptr<IOBuf>>> queued_bufs_;
//   };

//   static void Init();
//   static NetworkManager& HandleFault(EbbId id);
//   Interface& NewInterface(EthernetDevice& ether_dev);
//   Interface& FirstInterface();

//  private:
//   friend void ebbrt::Main(ebbrt::multiboot::Information* mbi);
//   void AcquireIPAddress();

//   std::vector<Interface> interfaces_;
// };
// constexpr auto network_manager = EbbRef<NetworkManager>(kNetworkManagerId);
// }  // namespace ebbrt
