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

#include <ebbrt/BootFdt.h>
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
  timer->Start(std::chrono::milliseconds(10),
               []() {
                 for (auto& itf : network_manager->interfaces_) {
                   itf.Poll();
                 }
                 sys_check_timeouts();
               },
               /* repeat = */ true);
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

ebbrt::NetworkManager::Interface& ebbrt::NetworkManager::FirstInterface() {
  if (interfaces_.empty())
    throw std::runtime_error("No Interfaces");
  return interfaces_.front();
}

#ifndef __EBBRT_ENABLE_STATIC_IP__
namespace {
ebbrt::EventManager::EventContext* context;
}
#endif

void ebbrt::NetworkManager::AcquireIPAddress() {
  kbugon(interfaces_.size() == 0, "No network interfaces identified!\n");
  auto netif = &interfaces_[0].netif_;
  netif_set_default(netif);
#ifdef __EBBRT_ENABLE_STATIC_IP__
  const auto& mac_addr = interfaces_[0].MacAddress();
  auto fdt = boot_fdt::Get();
  auto net = static_cast<uint8_t>(
      fdt.GetProperty16(fdt.GetNodeOffset("/runtime"), "net"));
  ip_addr_t addr;
  IP4_ADDR(&addr, 10, net, net,
           mac_addr[5]);  // set addr to 10.net.net.last_mac_octet
  ip_addr_t netmask;
  IP4_ADDR(&netmask, 255, 255, 255, 0);  // set netmask to 255.255.255.0
  ip_addr_t gw;
  IP4_ADDR(&gw, 10, net, net, 1);  // set gw to 10.net.net.1
  netif_set_addr(netif, &addr, &netmask, &gw);
  netif_set_up(netif);
#else
  dhcp_start(netif);
  context = new EventManager::EventContext;
  event_manager->SaveContext(*context);
#endif
}

namespace {
err_t EthOutput(struct netif* netif, struct pbuf* p) {
  auto itf = static_cast<ebbrt::NetworkManager::Interface*>(netif->state);

#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE);
#endif

  if (p == nullptr)
    return ERR_OK;

  pbuf_ref(p);
  auto b = ebbrt::IOBuf::TakeOwnership(p->payload, p->len,
                                       [p](void* pointer) { pbuf_free(p); });
  // TODO(dschatz): avoid this copy
  // auto b = ebbrt::IOBuf::Create(p->len);
  // std::memcpy(b->WritableData(), p->payload, p->len);

  for (struct pbuf* q = p->next; q != nullptr; q = q->next) {
    auto nbuf =
        ebbrt::IOBuf::TakeOwnership(q->payload, q->len, [](void* pointer) {});
    // auto nbuf = ebbrt::IOBuf::Create(q->len);
    // std::memcpy(nbuf->WritableData(), q->payload, q->len);
    b->Prev()->AppendChain(std::move(nbuf));
  }
  itf->Send(std::move(b));

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
#ifndef __EBBRT_ENABLE_STATIC_IP__
  ebbrt::event_manager->ActivateContext(std::move(*context));
  delete context;
#endif
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

void ebbrt::NetworkManager::Interface::Poll() { return ether_dev_.Poll(); }

const std::array<char, 6>& ebbrt::NetworkManager::Interface::MacAddress() {
  return ether_dev_.GetMacAddress();
}

uint32_t ebbrt::NetworkManager::Interface::IPV4Addr() {
  return netif_.ip_addr.addr;
}

void ebbrt::NetworkManager::Interface::Send(std::unique_ptr<const IOBuf>&& b) {
  ether_dev_.Send(std::move(b));
}

struct pbuf_custom_wrapper {
  pbuf_custom p_custom;
  std::unique_ptr<ebbrt::IOBuf> buf;
};

void ebbrt::NetworkManager::Interface::Receive(std::unique_ptr<IOBuf>&& buf) {
  kbugon(buf->CountChainElements() > 1, "Handle chained buffer\n");
  auto data = buf->WritableData() - ETH_PAD_SIZE;
  auto len = buf->ComputeChainDataLength() + ETH_PAD_SIZE;

  auto p_wrapper = new pbuf_custom_wrapper();
  p_wrapper->buf = std::move(buf);
  p_wrapper->p_custom.custom_free_function = [](pbuf* p) {
    auto wrapper = reinterpret_cast<pbuf_custom_wrapper*>(p);
    delete wrapper;
  };
  auto p = pbuf_alloced_custom(PBUF_RAW, len, PBUF_REF, &p_wrapper->p_custom,
                               data, len);
  kbugon(p == nullptr, "Failed to allocate pbuf\n");

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
  ebbrt::kprintf("\n");
  ebbrt::kabort();
}

extern "C" uint32_t lwip_rand() { return ebbrt::rdtsc() % 0xFFFFFFFF; }

u32_t sys_now() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             ebbrt::clock::Wall::Now().time_since_epoch()).count();
}

void ebbrt::NetworkManager::TcpPcb::Deleter(struct tcp_pcb* object) {
  if (object != nullptr)
    tcp_close(object);
}

ebbrt::NetworkManager::TcpPcb::TcpPcb()
    : pcb_(nullptr, &Deleter), sent_(0), next_(0) {
  auto pcb = tcp_new();
  if (pcb == nullptr) {
    throw std::bad_alloc();
  }
  pcb_ =
      std::unique_ptr<struct tcp_pcb, void (*)(struct tcp_pcb*)>(pcb, &Deleter);
  tcp_arg(pcb_.get(), static_cast<void*>(this));
}

ebbrt::NetworkManager::TcpPcb::TcpPcb(struct tcp_pcb* pcb)
    : pcb_(pcb, &Deleter), sent_(0), next_(0) {
  tcp_arg(pcb_.get(), static_cast<void*>(this));
  tcp_sent(pcb_.get(), SentHandler);
}

ebbrt::NetworkManager::TcpPcb::TcpPcb(TcpPcb&& other)
    : pcb_(std::move(other.pcb_)),
      accept_callback_(std::move(other.accept_callback_)),
      connect_promise_(std::move(other.connect_promise_)),
      receive_callback_(std::move(other.receive_callback_)),
      sent_(std::move(other.sent_)), next_(std::move(other.next_)),
      ack_queue_(std::move(other.ack_queue_)) {
  tcp_arg(pcb_.get(), static_cast<void*>(this));
}

ebbrt::NetworkManager::TcpPcb& ebbrt::NetworkManager::TcpPcb::
operator=(TcpPcb&& other) {
  pcb_ = std::move(other.pcb_);
  accept_callback_ = std::move(other.accept_callback_);
  connect_promise_ = std::move(other.connect_promise_);
  receive_callback_ = std::move(other.receive_callback_);
  sent_ = std::move(other.sent_);
  next_ = std::move(other.next_);
  ack_queue_ = std::move(other.ack_queue_);

  tcp_arg(pcb_.get(), static_cast<void*>(this));

  return *this;
}

void ebbrt::NetworkManager::TcpPcb::Bind(uint16_t port) {
  auto ret = tcp_bind(pcb_.get(), IP_ADDR_ANY, port);
  if (ret != ERR_OK) {
    throw std::runtime_error("Bind failed\n");
  }
}

void ebbrt::NetworkManager::TcpPcb::Listen() {
  auto pcb = tcp_listen(pcb_.get());
  if (pcb == NULL) {
    throw std::bad_alloc();
  }
  pcb_.release();
  pcb_.reset(pcb);
}

void ebbrt::NetworkManager::TcpPcb::ListenWithBacklog(uint8_t backlog) {
  auto pcb = tcp_listen_with_backlog(pcb_.get(), backlog);
  if (pcb == NULL) {
    throw std::bad_alloc();
  }
  pcb_.release();
  pcb_.reset(pcb);
}

void
ebbrt::NetworkManager::TcpPcb::Accept(std::function<void(TcpPcb)> callback) {
  accept_callback_ = std::move(callback);
  tcp_accept(pcb_.get(), Accept_Handler);
}

ebbrt::Future<void> ebbrt::NetworkManager::TcpPcb::Connect(struct ip_addr* ip,
                                                           uint16_t port) {
  auto err = tcp_connect(pcb_.get(), ip, port, Connect_Handler);
  if (err != ERR_OK) {
    throw std::bad_alloc();
  }
  return connect_promise_.GetFuture();
}

err_t ebbrt::NetworkManager::TcpPcb::Accept_Handler(void* arg,
                                                    struct tcp_pcb* newpcb,
                                                    err_t err) {
  kassert(err == ERR_OK);
  auto listening_pcb = static_cast<TcpPcb*>(arg);
  listening_pcb->accept_callback_(TcpPcb(newpcb));
  tcp_accepted(listening_pcb->pcb_.get());
  return ERR_OK;
}

err_t ebbrt::NetworkManager::TcpPcb::Connect_Handler(void* arg,
                                                     struct tcp_pcb* pcb,
                                                     err_t err) {
  kassert(err == ERR_OK);
  auto pcb_s = static_cast<TcpPcb*>(arg);
  kassert(pcb_s->pcb_.get() == pcb);
  tcp_sent(pcb_s->pcb_.get(), SentHandler);
  pcb_s->connect_promise_.SetValue();
  return ERR_OK;
}

void ebbrt::NetworkManager::TcpPcb::Receive(
    std::function<void(TcpPcb&, std::unique_ptr<IOBuf>&&)> callback) {
  if (!receive_callback_) {
    tcp_recv(pcb_.get(), Receive_Handler);
  }
  receive_callback_ = std::move(callback);
}

err_t ebbrt::NetworkManager::TcpPcb::Receive_Handler(void* arg,
                                                     struct tcp_pcb* pcb,
                                                     struct pbuf* p,
                                                     err_t err) {
  kassert(err == ERR_OK);
  auto pcb_s = static_cast<TcpPcb*>(arg);
  kassert(pcb_s->pcb_.get() == pcb);
  if (p == nullptr) {
    pcb_s->receive_callback_(*pcb_s, IOBuf::Create(0));
  } else {
    auto b = IOBuf::TakeOwnership(p->payload, p->len,
                                  [p](void* pointer) { pbuf_free(p); });
    for (auto q = p->next; q != nullptr; q = q->next) {
      auto n = IOBuf::TakeOwnership(q->payload, q->len, [](void* pointer) {});
      b->Prev()->AppendChain(std::move(n));
    }
    pcb_s->receive_callback_(*pcb_s, std::move(b));
    tcp_recved(pcb_s->pcb_.get(), p->tot_len);
  }
  return ERR_OK;
}  // NOLINT

size_t ebbrt::NetworkManager::TcpPcb::InternalSend(
    const std::unique_ptr<IOBuf>& data) {
  size_t ret = 0;
  for (auto& buf : *data) {
    while (buf.Length() != 0) {
      auto sndbuf = tcp_sndbuf(pcb_.get());
      if (unlikely(sndbuf == 0))
        return ret;
      auto sz = buf.Length();
      sz = sz > UINT16_MAX ? UINT16_MAX : sz;
      sz = sz > sndbuf ? sndbuf : sz;
      uint8_t flag = 0;
      if (sz < sndbuf && buf.Next() != &*data) {
        // we expect to write more almost immediately if there is room in the
        // send buf and this is not the last element of the chain
        flag = TCP_WRITE_FLAG_MORE;
      }
      auto err =
          tcp_write(pcb_.get(), const_cast<uint8_t*>(buf.Data()), sz, flag);
      kassert(err == ERR_OK);
      buf.Advance(sz);
      ret += sz;
    }
  }
  return ret;
}

ebbrt::Future<void>
ebbrt::NetworkManager::TcpPcb::Send(std::unique_ptr<IOBuf>&& data) {
  auto len = data->ComputeChainDataLength();
  size_t written = 0;
  if (likely(queued_bufs_.empty())) {
    written = InternalSend(data);
  }
  // The IOBuf is stored in the ack map until LWIP tells us it has been sent
  next_ += len;
  ack_queue_.emplace(next_, Promise<void>(), std::move(data));
  auto& p = ack_queue_.back();
  auto& d = std::get<2>(p);
  if (written < len) {
    queued_bufs_.emplace(std::ref(d));
  }
  return std::get<1>(p).GetFuture();
}

void ebbrt::NetworkManager::TcpPcb::EnableNagle() { tcp_nagle_enable(pcb_); }
void ebbrt::NetworkManager::TcpPcb::DisableNagle() { tcp_nagle_disable(pcb_); }
bool ebbrt::NetworkManager::TcpPcb::NagleDisabled() {
  return tcp_nagle_disabled(pcb_);
}

err_t ebbrt::NetworkManager::TcpPcb::SentHandler(void* arg, struct tcp_pcb* pcb,
                                                 uint16_t len) {
  auto pcb_s = static_cast<TcpPcb*>(arg);
  kassert(pcb_s->pcb_.get() == pcb);
  pcb_s->sent_ += len;
  while (!pcb_s->ack_queue_.empty() &&
         std::get<0>(pcb_s->ack_queue_.front()) <= pcb_s->sent_) {
    std::get<1>(pcb_s->ack_queue_.front()).SetValue();
    pcb_s->ack_queue_.pop();
  }
  while (!pcb_s->queued_bufs_.empty()) {
    auto& buf = pcb_s->queued_bufs_.front().get();
    auto buf_len = buf->ComputeChainDataLength();
    auto written = pcb_s->InternalSend(buf);
    if (written < buf_len)
      break;
    pcb_s->queued_bufs_.pop();
  }
  return ERR_OK;
}

ip_addr_t ebbrt::NetworkManager::TcpPcb::GetRemoteAddress() const {
  return pcb_->remote_ip;
}

uint16_t ebbrt::NetworkManager::TcpPcb::GetRemotePort() const {
  return pcb_->remote_port;
}
