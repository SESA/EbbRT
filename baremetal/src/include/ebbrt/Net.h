//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NET_H_

#include <ebbrt/AtomicUniquePtr.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/IOBuf.h>
#include <ebbrt/NetDhcp.h>
#include <ebbrt/NetEth.h>
#include <ebbrt/NetIp.h>
#include <ebbrt/PoolAllocator.h>
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
  struct UdpEntry {
    RcuHListHook hook;
    uint16_t port{0};
    MovableFunction<void(Ipv4Address, uint16_t, std::unique_ptr<MutIOBuf>)>
    func;
    std::atomic<bool> reading{false};
  };

  class Interface;

  class UdpPcb {
   public:
    UdpPcb() : entry_{new UdpEntry()} {}
    uint16_t Bind(uint16_t port);
    void Receive(MovableFunction<
        void(Ipv4Address, uint16_t, std::unique_ptr<MutIOBuf>)> func);
    void SendTo(Ipv4Address addr, uint16_t port, std::unique_ptr<IOBuf> buf);

   private:
    struct UdpEntryDeleter {
      void operator()(UdpEntry* e);
    };
    std::unique_ptr<UdpEntry, UdpEntryDeleter> entry_;

    friend class Interface;
  };

  class Interface {
   public:
    struct ItfAddress {
      bool isBroadcast(const Ipv4Address& addr) const;
      bool isLocalNetwork(const Ipv4Address& addr) const;

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
    void SendUdp(UdpPcb& pcb, Ipv4Address addr, uint16_t port,
                 std::unique_ptr<IOBuf> buf);
    void SendIp(std::unique_ptr<MutIOBuf> buf, Ipv4Address src, Ipv4Address dst,
                uint8_t proto);
    void Poll();
    const EthernetAddress& MacAddress();
    const ItfAddress* Address() const { return address_.get(); }
    void SetAddress(std::unique_ptr<ItfAddress> address) {
      address_.store(address.release());
    }
    Future<void> StartDhcp();

   private:
    struct DhcpPcb : public CacheAligned {
      std::mutex lock;
      UdpPcb udp_pcb;
      enum State { kInactive, kSelecting, kRequesting, kBound } state;
      uint32_t xid;
      size_t tries;
      size_t options_len;
      Promise<void> complete;
    };

    void ReceiveArp(EthernetHeader& eh, std::unique_ptr<MutIOBuf> buf);
    void ReceiveIp(EthernetHeader& eh, std::unique_ptr<MutIOBuf> buf);
    void ReceiveIcmp(EthernetHeader& eh, Ipv4Header& ih,
                     std::unique_ptr<MutIOBuf> buf);
    void ReceiveUdp(Ipv4Header& ih, std::unique_ptr<MutIOBuf> buf);
    void ReceiveDhcp(Ipv4Address from_addr, uint16_t from_port,
                     std::unique_ptr<MutIOBuf> buf);
    void EthArpSend(uint16_t proto, const Ipv4Header& ih,
                    std::unique_ptr<MutIOBuf> buf);
    void EthArpRequest(ArpEntry& entry);
    void DhcpOption(DhcpMessage& message, uint8_t option_type,
                    uint8_t option_len);
    void DhcpOptionByte(DhcpMessage& message, uint8_t value);
    void DhcpOptionShort(DhcpMessage& message, uint16_t value);
    void DhcpOptionLong(DhcpMessage& message, uint32_t value);
    boost::optional<uint8_t> DhcpGetOptionByte(const DhcpMessage& message,
                                               uint8_t option_type);
    boost::optional<uint16_t> DhcpGetOptionShort(const DhcpMessage& message,
                                                 uint8_t option_type);
    boost::optional<uint32_t> DhcpGetOptionLong(const DhcpMessage& message,
                                                uint8_t option_type);
    void DhcpOptionTrailer(DhcpMessage& message);
    void DhcpSetState(DhcpPcb::State state);
    void DhcpCreateMessage(DhcpMessage& msg);
    void DhcpDiscover();
    void DhcpHandleOffer(const DhcpMessage& message);
    void DhcpHandleAck(const DhcpMessage& message);

    atomic_unique_ptr<ItfAddress, ItfAddressDeleter> address_;
    EthernetDevice& ether_dev_;
    DhcpPcb dhcp_pcb_;
  };

  static void Init();

  Interface& NewInterface(EthernetDevice& ether_dev);

 private:
  Future<void> StartDhcp();
  Interface* IpRoute(Ipv4Address dest);

  std::unique_ptr<Interface> interface_;
  RcuHashTable<ArpEntry, Ipv4Address, &ArpEntry::hook, &ArpEntry::paddr>
  arp_cache_{8};  // 256 buckets
  RcuHashTable<UdpEntry, uint16_t, &UdpEntry::hook, &UdpEntry::port> udp_pcbs_{
      8};  // 256 buckets
  EbbRef<PoolAllocator<uint16_t, 25>> port_allocator_{
      PoolAllocator<uint16_t, 25>::Create(49152, 65535,
                                          ebb_allocator->AllocateLocal())};

  alignas(cache_size) std::mutex arp_write_lock_;
  alignas(cache_size) std::mutex udp_write_lock_;

  friend void ebbrt::Main(ebbrt::multiboot::Information* mbi);
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
