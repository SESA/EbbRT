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

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "app/app.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/EventManager/EventManager.hpp"
#include "ebb/Network/SocketNetwork.hpp"

ebbrt::EbbRoot *ebbrt::SocketNetwork::ConstructRoot() {
  return new SharedRoot<SocketNetwork>();
}

// registers symbol for configuration
__attribute__((constructor(65535))) static void _reg_symbol() {
  ebbrt::app::AddSymbol("Network", ebbrt::SocketNetwork::ConstructRoot);
}

ebbrt::SocketNetwork::SocketNetwork(EbbId id) : Network{ id } {
  if ((send_fd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("cannot create socket");
    throw std::runtime_error("Socket creation failed");
  }

  struct sockaddr_in addr;
  memset((char *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(0);

  if (bind(send_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind failed");
    throw std::runtime_error("Bind failed");
  }
}

void ebbrt::SocketNetwork::InitPing() { assert(0); }

void ebbrt::SocketNetwork::InitEcho() { assert(0); }

#include <iostream>

uint16_t
ebbrt::SocketNetwork::RegisterUDP(uint16_t port,
                                  std::function<void(Buffer, NetworkId)> cb) {
  int fd;
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("cannot create socket");
    throw std::runtime_error("Socket creation failed");
  }

  struct sockaddr_in addr;
  memset((char *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind failed");
    throw std::runtime_error("Bind failed");
  }

  socklen_t addr_len = sizeof(addr);
  if (getsockname(fd, (struct sockaddr *)&addr, &addr_len) < 0) {
    perror("getsockname failed");
    throw std::runtime_error("Getsockname failed");
  }

  auto ret = ntohs(addr.sin_port);

  auto interrupt = event_manager->AllocateInterrupt([=]() {
    const constexpr size_t UDP_PACKET_SIZE = 65535;
    auto buf = std::malloc(UDP_PACKET_SIZE);
    if (buf == nullptr) {
      throw std::bad_alloc();
    }
    sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    auto len = recvfrom(fd, buf, UDP_PACKET_SIZE, MSG_DONTWAIT,
                        (struct sockaddr *)&remote_addr, &addr_len);
    if (len < 0) {
      perror("recv failed");
      throw std::runtime_error("Recv failed");
    }

    NetworkId id;
    id.addr = ntohl(remote_addr.sin_addr.s_addr);
    cb(Buffer(buf, len), id);
  });

  event_manager->RegisterFD(fd, EPOLLIN, interrupt);
  return ret;
}

void ebbrt::SocketNetwork::SendUDP(Buffer buffer, NetworkId to, uint16_t port) {
  struct sockaddr_in target;
  target.sin_family = AF_INET;
  target.sin_addr.s_addr = htonl(to.addr);
  target.sin_port = htons(port);
  if (sendto(send_fd_, buffer.data(), buffer.length(), 0,
             (struct sockaddr *)&target, sizeof(target)) < 0) {
    perror("sendto failed");
    throw std::runtime_error("sendto failed");
  }
}
