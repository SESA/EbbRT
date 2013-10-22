/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>

#include <cstdarg>
#include <iostream>
#include <queue>
#include <stdexcept>

#include "app/app.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/Ethernet/Ethernet.hpp"
#include "ebb/Network/LWIPNetwork.hpp"
#include "ebb/Timer/Timer.hpp"

#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/inet_chksum.h"
#include "lwip/init.h"
#include "lwip/ip_addr.h"
#include "lwip/ip_frag.h"
#include "lwip/raw.h"
#include "lwip/stats.h"
#include "lwip/tcp_impl.h"
#include "netif/etharp.h"

ebbrt::EbbRoot *ebbrt::LWIPNetwork::ConstructRoot() {
  return new SharedRoot<LWIPNetwork>();
}

// registers symbol for configuration
__attribute__((constructor(65535))) static void _reg_symbol() {
  ebbrt::app::AddSymbol("Network", ebbrt::LWIPNetwork::ConstructRoot);
}

namespace {
err_t eth_output(struct netif *netif, struct pbuf *p) {
  try {

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);
#endif

    u16_t len = 0;
    for (struct pbuf *q = p; q != NULL; q = q->next) {
      len += q->len;
    }

    // std::cout << "Sending " << len << std::endl;

    auto buf = ebbrt::ethernet->Alloc(len);
    char *location = buf.data();
    for (struct pbuf *q = p; q != NULL; q = q->next) {
      memcpy(location, q->payload, q->len);
      location += q->len;
    }
    ebbrt::ethernet->Send(std::move(buf));

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);
#endif

    LINK_STATS_INC(link.xmit);

    return ERR_OK;
  }
  catch (std::exception &e) {
    // std::cerr << "Sending packet failed: " << e.what() << std::endl;
  }
  catch (...) {
    // std::cerr << "Sending packet failed" << std::endl;
  }
  return ERR_IF;
}

err_t eth_init(struct netif *netif) {
  LRT_ASSERT(netif != NULL);

  netif->hwaddr_len = 6;
  auto macaddr = ebbrt::ethernet->MacAddress();
  memcpy(netif->hwaddr, macaddr, 6);
  netif->mtu = 1500;
  netif->name[0] = 'e';
  netif->name[1] = 'n';
  netif->output = etharp_output;
  netif->linkoutput = eth_output;
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  return ERR_OK;
}

namespace {
bool has_dhcp = false;
}
void status_callback(struct netif *netif) {
  has_dhcp = true;
  char buf[80];
  sprintf(buf, "%" U16_F ".%" U16_F ".%" U16_F ".%" U16_F "\n",
          ip4_addr1_16(&netif->ip_addr), ip4_addr2_16(&netif->ip_addr),
          ip4_addr3_16(&netif->ip_addr), ip4_addr4_16(&netif->ip_addr));
  ebbrt::lrt::console::write(buf);
  static_cast<ebbrt::EbbRef<ebbrt::LWIPNetwork> >(ebbrt::network)->EmptyQueue();
}
}

extern "C" int lwip_printf(const char *fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  char buf[1024];
  ret = vsprintf(buf, fmt, ap);
  va_end(ap);
  ebbrt::lrt::console::write(buf);
  return ret;
}

static struct netif netif_;

ebbrt::LWIPNetwork::LWIPNetwork(EbbId id) : Network{ id } {
  auto f = [=](Buffer buffer) {
    auto me = static_cast<EbbRef<LWIPNetwork> >(ebbid_);
    me->RecvPacket(std::move(buffer));
  };

  ethernet->Register(0x0800, f);
  ethernet->Register(0x0806, f);
  lwip_init();

  lrt::console::write("LWIP Inited\n");
  if (netif_add(&netif_, nullptr, nullptr, nullptr, nullptr, eth_init,
                ethernet_input) == nullptr) {
    throw std::runtime_error("Failed to create interface");
  }
  netif_set_default(&netif_);

  netif_set_status_callback(&netif_, status_callback);
  dhcp_start(&netif_);
  udp_bind(&send_pcb_, IP_ADDR_ANY, 0);

  tcp_timer_func_ = [&]() {
    tcp_tmr();
    timer->Wait(std::chrono::milliseconds{ TCP_TMR_INTERVAL }, tcp_timer_func_);
  };
  ip_timer_func_ = [&]() {
    ip_reass_tmr();
    timer->Wait(std::chrono::milliseconds{ IP_TMR_INTERVAL }, ip_timer_func_);
  };
  arp_timer_func_ = [&]() {
    etharp_tmr();
    timer->Wait(std::chrono::milliseconds{ ARP_TMR_INTERVAL }, arp_timer_func_);
  };
  dhcp_coarse_timer_func_ = [&]() {
    dhcp_coarse_tmr();
    timer->Wait(std::chrono::milliseconds{ DHCP_COARSE_TIMER_MSECS },
                dhcp_coarse_timer_func_);
  };
  dhcp_fine_timer_func_ = [&]() {
    dhcp_fine_tmr();
    timer->Wait(std::chrono::milliseconds{ DHCP_FINE_TIMER_MSECS },
                dhcp_fine_timer_func_);
  };
  dns_timer_func_ = [&]() {
    dns_tmr();
    timer->Wait(std::chrono::milliseconds{ DNS_TMR_INTERVAL }, dns_timer_func_);
  };

  timer->Wait(std::chrono::milliseconds{ TCP_TMR_INTERVAL }, tcp_timer_func_);
#if IP_REASSEMBLY
  timer->Wait(std::chrono::milliseconds{ IP_TMR_INTERVAL }, ip_timer_func_);
#endif

#if LWIP_ARP
  timer->Wait(std::chrono::milliseconds{ ARP_TMR_INTERVAL }, arp_timer_func_);
#endif

#if LWIP_DHCP
  timer->Wait(std::chrono::milliseconds{ DHCP_COARSE_TIMER_MSECS },
              dhcp_coarse_timer_func_);
  timer->Wait(std::chrono::milliseconds{ DHCP_FINE_TIMER_MSECS },
              dhcp_fine_timer_func_);
#endif

#if LWIP_DNS
  timer->Wait(std::chrono::milliseconds{ DNS_TMR_INTERVAL }, dns_timer_func_);
#endif
}

void ebbrt::LWIPNetwork::RecvPacket(Buffer buffer) {
  auto p = pbuf_alloc(PBUF_LINK, buffer.length() + 2, PBUF_POOL);

  LRT_ASSERT(p != NULL);

  auto ptr = buffer.data();
  bool first = true;
  for (auto q = p; q != NULL; q = q->next) {
    auto add = 0;
    if (first) {
      add = 2;
      first = false;
    }
    memcpy(static_cast<char *>(q->payload) + add, ptr, q->len - add);
    ptr += q->len;
  }

  netif_.input(p, &netif_);
}

namespace {
u8_t ping_recv(void *arg, struct raw_pcb *upcb, struct pbuf *p,
               struct ip_addr *addr) {
  LRT_ASSERT(p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr)));
  LRT_ASSERT(pbuf_header(p, -PBUF_IP_HLEN) == 0);
  auto iecho = static_cast<struct icmp_echo_hdr *>(p->payload);

  ICMPH_TYPE_SET(iecho, ICMP_ER);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;
  iecho->chksum = inet_chksum(iecho, p->len);

  raw_sendto(upcb, p, addr);
  pbuf_free(p);
  return 1;
}
}

void ebbrt::LWIPNetwork::InitPing() {
  auto ping_pcb = raw_new(IP_PROTO_ICMP);
  LRT_ASSERT(ping_pcb != nullptr);
  raw_recv(ping_pcb, ping_recv, nullptr);
  raw_bind(ping_pcb, IP_ADDR_ANY);
}

extern "C" void echo_init();

void ebbrt::LWIPNetwork::InitEcho() { echo_init(); }

void
ebbrt::LWIPNetwork::RegisterUDP(uint16_t port,
                                std::function<void(Buffer, NetworkId)> cb) {
  auto udp_pcb = udp_new();
  LRT_ASSERT(udp_pcb != nullptr);
  udp_bind(udp_pcb, IP_ADDR_ANY, port);
  auto heap_cb = new std::function<void(Buffer, NetworkId)>(std::move(cb));
  udp_recv(
      udp_pcb,
      [](void * arg, struct udp_pcb * upcb, struct pbuf * p,
         struct ip_addr * addr, uint16_t port) {
        auto cb =
            reinterpret_cast<std::function<void(Buffer, NetworkId)> *>(arg);
        NetworkId id;
        id.addr = addr->addr;
        size_t len = 0;
        for (struct pbuf *q = p; q != NULL; q = q->next) {
          len += q->len;
        }

        auto buf = static_cast<char *>(std::malloc(len));
        if (buf == nullptr) {
          throw std::bad_alloc();
        }
        auto location = buf;
        for (struct pbuf *q = p; q != NULL; q = q->next) {
          std::memcpy(location, q->payload, q->len);
          location += q->len;
        }
        (*cb)(Buffer(buf, len), id);
      },
      heap_cb);
}

void ebbrt::LWIPNetwork::SendUDP(Buffer buffer, NetworkId to, uint16_t port) {
  if (!has_dhcp) {
    queue_.push(
        std::make_tuple(std::move(buffer), std::move(to), std::move(port)));
  } else {
    auto p = pbuf_alloc(PBUF_TRANSPORT, buffer.length(), PBUF_POOL);
    LRT_ASSERT(p != nullptr);

    auto ptr = buffer.data();
    for (auto q = p; q != nullptr; q = q->next) {
      std::memcpy(static_cast<char *>(q->payload), ptr, q->len);
      ptr += q->len;
    }

    struct ip_addr addr;
    IP4_ADDR(&addr, (to.addr >> 24) & 0xFF, (to.addr >> 16) & 0xFF,
             (to.addr >> 8) & 0xFF, to.addr & 0xFF);
    udp_sendto(&send_pcb_, p, &addr, port);

    pbuf_free(p);
  }
}

void ebbrt::LWIPNetwork::EmptyQueue() {
  while (!queue_.empty()) {
    auto &tup = queue_.front();
    SendUDP(std::get<0>(std::move(tup)), std::get<1>(std::move(tup)),
            std::get<2>(std::move(tup)));
    queue_.pop();
  }
}
