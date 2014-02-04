//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Net.h>

#include <cstring>

#include <lwip/dhcp.h>
#include <lwip/init.h>
#include <lwip/stats.h>
#include <lwip/sys.h>
#include <lwip/tcp.h>
#include <lwip/timers.h>
#include <netif/etharp.h>

#include <ebbrt/Clock.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/ExplicitlyConstructed.h>
#include <ebbrt/Rdtsc.h>
#include <ebbrt/Timer.h>

namespace {
ebbrt::ExplicitlyConstructed<ebbrt::NetworkManager> the_manager;
}

void ebbrt::NetworkManager::Init() {
  the_manager.construct();
  lwip_init();
  timer->Start(std::chrono::milliseconds(10), []() { sys_check_timeouts(); });
}

ebbrt::NetworkManager& ebbrt::NetworkManager::HandleFault(EbbId id) {
  kassert(id == kNetworkManagerId);
  auto& ref = *the_manager;
  EbbRef<NetworkManager>::CacheRef(id, ref);
  return ref;
}

ebbrt::NetworkManager::Interface&
ebbrt::NetworkManager::NewInterface(EthernetDevice& ether_dev) {
  interfaces_.emplace_back(ether_dev, interfaces_.size());
  return interfaces_[interfaces_.size() - 1];
}

namespace {
ebbrt::EventManager::EventContext* context;
}

void ebbrt::NetworkManager::AcquireIPAddress() {
  kbugon(interfaces_.size() == 0, "No network interfaces identified!\n");
  netif_set_default(&interfaces_[0].netif_);
  dhcp_start(&interfaces_[0].netif_);
  context = new EventManager::EventContext;
  event_manager->SaveContext(*context);
}

namespace {
err_t EthOutput(struct netif* netif, struct pbuf* p) {
  auto itf = static_cast<ebbrt::NetworkManager::Interface*>(netif->state);

#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE);
#endif

  ebbrt::ConstBufferList l;
  if (p == nullptr)
    return ERR_OK;

  auto ptr = std::malloc(p->len);
  if (ptr == nullptr)
    throw std::bad_alloc();
  std::memcpy(ptr, p->payload, p->len);
  l.emplace_front(ptr, p->len);
  auto it = l.begin();
  for (struct pbuf* q = p->next; q != nullptr; q = q->next) {
    auto ptr = std::malloc(q->len);
    if (ptr == nullptr)
      throw std::bad_alloc();

    std::memcpy(ptr, q->payload, q->len);
    it = l.emplace_after(it, ptr, q->len,
                         [](const void* p) { free(const_cast<void*>(p)); });
  }
  itf->Send(std::move(l));

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE);
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

err_t EthInit(struct netif* netif) {
  auto itf = static_cast<ebbrt::NetworkManager::Interface*>(netif->state);
  netif->hwaddr_len = 6;
  memcpy(netif->hwaddr, itf->MacAddress().data(), 6);
  netif->mtu = 1500;
  netif->name[0] = 'e';
  netif->name[1] = 'n';
  netif->output = etharp_output;
  netif->linkoutput = EthOutput;
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
  return ERR_OK;
}

void StatusCallback(struct netif* netif) {
  ebbrt::kprintf("IP address: %" U16_F ".%" U16_F ".%" U16_F ".%" U16_F "\n",
                 ip4_addr1_16(&netif->ip_addr), ip4_addr2_16(&netif->ip_addr),
                 ip4_addr3_16(&netif->ip_addr), ip4_addr4_16(&netif->ip_addr));
  ebbrt::event_manager->ActivateContext(*context);
  delete context;
}
}  // namespace

ebbrt::NetworkManager::Interface::Interface(EthernetDevice& ether_dev,
                                            size_t idx)
    : ether_dev_(ether_dev), idx_(idx) {
  if (netif_add(&netif_, nullptr, nullptr, nullptr, static_cast<void*>(this),
                EthInit, ethernet_input) == nullptr) {
    throw std::runtime_error("Failed to create network interface");
  }
  netif_set_status_callback(&netif_, StatusCallback);
}

const std::array<char, 6>& ebbrt::NetworkManager::Interface::MacAddress() {
  return ether_dev_.GetMacAddress();
}

void ebbrt::NetworkManager::Interface::Send(ConstBufferList l) {
  ether_dev_.Send(std::move(l));
}

void ebbrt::NetworkManager::Interface::ReceivePacket(MutableBuffer buf,
                                                     uint32_t len) {
  // FIXME: We should avoid this copy
  auto p = pbuf_alloc(PBUF_LINK, len + ETH_PAD_SIZE, PBUF_POOL);

  kbugon(p == nullptr, "Failed to allocate pbuf");

  auto ptr = buf.addr();
  bool first = true;
  for (auto q = p; q != nullptr; q = q->next) {
    auto add = 0;
    if (first) {
      add = ETH_PAD_SIZE;
      first = false;
    }
    memcpy(static_cast<char*>(q->payload) + add, ptr, q->len - add);
    ptr = static_cast<void*>(static_cast<char*>(ptr) + q->len);
  }

  event_manager->SpawnLocal([p, this]() { netif_.input(p, &netif_); });
}

extern "C" void lwip_printf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  ebbrt::kvprintf(fmt, ap);
  va_end(ap);
}

extern "C" void lwip_assert(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  ebbrt::kvprintf(fmt, ap);
  va_end(ap);
  ebbrt::kabort();
}

extern "C" uint32_t lwip_rand() { return ebbrt::rdtsc() % 0xFFFFFFFF; }

u32_t sys_now() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             ebbrt::clock::Time()).count();
}

ebbrt::NetworkManager::TcpPcb::TcpPcb() {
  pcb_ = tcp_new();
  if (pcb_ == nullptr) {
    throw std::bad_alloc();
  }
  tcp_arg(pcb_, static_cast<void*>(this));
}

ebbrt::NetworkManager::TcpPcb::TcpPcb(struct tcp_pcb* pcb) : pcb_(pcb) {
  tcp_arg(pcb_, static_cast<void*>(this));
}

ebbrt::NetworkManager::TcpPcb::~TcpPcb() {
  if (pcb_ != nullptr)
    tcp_abort(pcb_);
}

void ebbrt::NetworkManager::TcpPcb::Bind(uint16_t port) {
  auto ret = tcp_bind(pcb_, IP_ADDR_ANY, port);
  if (ret != ERR_OK) {
    throw std::runtime_error("Bind failed\n");
  }
}

void ebbrt::NetworkManager::TcpPcb::Listen() {
  auto pcb = tcp_listen(pcb_);
  if (pcb == NULL) {
    throw std::bad_alloc();
  }
  pcb_ = pcb;
}

void
ebbrt::NetworkManager::TcpPcb::Accept(std::function<void(TcpPcb)> callback) {
  accept_callback_ = std::move(callback);
  tcp_accept(pcb_, Accept_Handler);
}

void ebbrt::NetworkManager::TcpPcb::Connect(struct ip_addr* ip, uint16_t port,
                                            std::function<void()> callback) {
  connect_callback_ = std::move(callback);
  auto err = tcp_connect(pcb_, ip, port, Connect_Handler);
  if (err != ERR_OK) {
    throw std::bad_alloc();
  }
}

err_t ebbrt::NetworkManager::TcpPcb::Accept_Handler(void* arg,
                                                    struct tcp_pcb* newpcb,
                                                    err_t err) {
  kassert(err == ERR_OK);
  auto listening_pcb = static_cast<TcpPcb*>(arg);
  listening_pcb->accept_callback_(TcpPcb(newpcb));
  tcp_accepted(listening_pcb->pcb_);
  return ERR_OK;
}

err_t ebbrt::NetworkManager::TcpPcb::Connect_Handler(void* arg,
                                                     struct tcp_pcb* pcb,
                                                     err_t err) {
  kassert(err == ERR_OK);
  auto pcb_s = static_cast<TcpPcb*>(arg);
  kassert(pcb_s->pcb_ == pcb);
  pcb_s->connect_callback_();
  return ERR_OK;
}

void ebbrt::NetworkManager::TcpPcb::Receive(
    std::function<void(MutableBufferList)> callback) {
  receive_callback_ = std::move(callback);
  tcp_recv(pcb_, Receive_Handler);
}

err_t ebbrt::NetworkManager::TcpPcb::Receive_Handler(void* arg,
                                                     struct tcp_pcb* pcb,
                                                     struct pbuf* p,
                                                     err_t err) {
  kassert(err == ERR_OK);
  auto pcb_s = static_cast<TcpPcb*>(arg);
  kassert(pcb_s->pcb_ == pcb);
  MutableBufferList l;
  if (p == nullptr) {
    pcb_s->receive_callback_(std::move(l));
  } else {
    l.emplace_front(p->payload, p->len, [p](void* pointer) { pbuf_free(p); });
    auto it = l.begin();
    for (auto q = p->next; q != nullptr; q = q->next) {
      it = l.emplace_after(it, q->payload, q->len,
                           [q](void* pointer) { pbuf_free(q); });
    }
    pcb_s->receive_callback_(std::move(l));
    tcp_recved(pcb_s->pcb_, p->tot_len);
  }
  return ERR_OK;
}  // NOLINT
