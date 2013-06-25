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
#include <cassert>
#include <cstring>

#include <sys/socket.h>
#include <netpacket/packet.h>
#include <pcap.h>
#include <netinet/in.h>

#include "ebb/SharedRoot.hpp"
#include "ebb/Ethernet/RawSocket.hpp"
#include "ebb/EventManager/EventManager.hpp"

namespace {
  char dev[] = "tap100";
}

ebbrt::EbbRoot*
ebbrt::RawSocket::ConstructRoot()
{
  return new SharedRoot<RawSocket>();
}

ebbrt::RawSocket::RawSocket()
{
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_if_t* alldevs;

  int ret = pcap_findalldevs(&alldevs, errbuf);
  assert(ret != -1);

  while (alldevs != NULL) {
    if (strcmp(alldevs->name, dev) == 0) {
      break;
    }
    alldevs = alldevs->next;
  }

  assert(alldevs != NULL);

  auto addr = alldevs->addresses;
  while (addr != NULL) {
    if (addr->addr->sa_family == AF_PACKET) {
      break;
    }
    addr = addr->next;
  }
  assert(addr != NULL);

  auto packet_addr = reinterpret_cast<struct sockaddr_ll*>(addr->addr);
  std::copy(&packet_addr->sll_addr[0], &packet_addr->sll_addr[5], mac_addr_);

  pdev_ = pcap_open_live(dev, 65535, 0, 0, errbuf);
  assert(dev != NULL);

  ret = pcap_setnonblock(pdev_, 1, errbuf);
  assert(ret != -1);

  int fd = pcap_get_selectable_fd(pdev_);
  assert(fd != -1);

  uint8_t interrupt = event_manager->AllocateInterrupt([]() {
      ethernet->Receive();
    });
  event_manager->RegisterFD(fd, EPOLLIN, interrupt);
}

void
ebbrt::RawSocket::Send(BufferList buffers,
                       std::function<void()> cb)
{
  const void* buf;
  size_t len;
  char* aggregate;
  if (buffers.size() == 1) {
    buf = buffers.front().first;
    len = buffers.front().second;
  } else {
    len = 0;
    for (const auto& buffer : buffers) {
      len += buffer.second;
    }
    aggregate = new char[len];
    char* location = aggregate;
    for (const auto& buffer : buffers) {
      memcpy(location, buffer.first, buffer.second);
      location += buffer.second;
    }
    buf = aggregate;
  }
  pcap_inject(pdev_, buf, len);
  if (buffers.size() != 1) {
    free(aggregate);
  }
  if (cb) {
    event_manager->Async(std::move(cb));
  }
}

const uint8_t*
ebbrt::RawSocket::MacAddress()
{
  return mac_addr_;
}

void
ebbrt::RawSocket::SendComplete()
{
  assert(0);
}

void
ebbrt::RawSocket::Register(uint16_t ethertype,
                           std::function<void(const char*, size_t)> func)
{
  map_[ethertype] = func;
}

void
ebbrt::RawSocket::Receive()
{
  struct pcap_pkthdr* header;
  const uint8_t* data;
  while (pcap_next_ex(pdev_, &header, &data) == 1) {
    assert(header->caplen >= 14);
    uint16_t ethertype = ntohs(*reinterpret_cast<const uint16_t*>(&data[12]));
    auto it = map_.find(ethertype);
    if (it != map_.end()) {
      it->second(reinterpret_cast<const char*>(data), header->caplen);
    }
  }
}
