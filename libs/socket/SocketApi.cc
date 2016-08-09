//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "SocketManager.h"
#include "Vfs.h"
#include <ebbrt/Clock.h>
#include <ebbrt/Debug.h>
#include <ebbrt/NetMisc.h>
#include <ebbrt/Runtime.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>

#include "lwip/ip_addr.h"

int lwip_listen(int s, int backlog) {
  // TODO(jmc): support backlog 
  try {
    auto fd = ebbrt::root_vfs->Lookup(s);
    return static_cast<ebbrt::EbbRef<ebbrt::SocketManager::SocketFd>>(fd)
        ->Listen();
  } catch (std::invalid_argument& e) {
    errno = EBADF;
    return -1;
  }
}

int lwip_accept(int s, struct sockaddr* addr, socklen_t* addrlen) {
  try {
    auto fd = ebbrt::root_vfs->Lookup(s);
    return static_cast<ebbrt::EbbRef<ebbrt::SocketManager::SocketFd>>(fd)
        ->Accept()
        .Block()
        .Get();
  } catch (std::invalid_argument& e) {
    errno = EBADF;
    return -1;
  }
}

int lwip_bind(int s, const struct sockaddr* name, socklen_t namelen) {
  try {
    auto fd = ebbrt::root_vfs->Lookup(s);
    auto saddr = reinterpret_cast<const struct sockaddr_in*>(name);
    auto port = ebbrt::ntohs(saddr->sin_port);  // port arrives in network order
    return static_cast<ebbrt::EbbRef<ebbrt::SocketManager::SocketFd>>(fd)->Bind(
        port);
  } catch (std::invalid_argument& e) {
    errno = EBADF;
    return -1;
  }
}

int lwip_connect(int s, const struct sockaddr* name, socklen_t namelen) {
  // TODO(jmc): verify socket domain/type is valid
  auto saddr = reinterpret_cast<const struct sockaddr_in*>(name);
  auto ip_addr = saddr->sin_addr.s_addr;  // ip arrives in network order
  auto port = ebbrt::ntohs(saddr->sin_port);  // port arrives in network order
  ebbrt::NetworkManager::TcpPcb pcb;
  pcb.Connect(ebbrt::Ipv4Address(ip_addr), port);
  auto fd = ebbrt::root_vfs->Lookup(s);
  // TODO(jmc): verify fd type for connecting
  auto connection =
      static_cast<ebbrt::EbbRef<ebbrt::SocketManager::SocketFd>>(fd)
          ->Connect(std::move(pcb))
          .Block();
  auto is_connected = connection.Get();
  if (is_connected) {
    return 0;
  }
  return -1;
}

int lwip_socket(int domain, int type, int protocol) {
  if (domain != AF_INET || type != SOCK_STREAM ||
      (protocol != IPPROTO_IP && protocol != IPPROTO_TCP)) {
    ebbrt::kabort("Socket type not supported");
    return -1;
  }
  // ebbrt::unix::socket::ipv4->New()
  return ebbrt::socket_manager->NewIpv4Socket();
}

int lwip_read(int s, void* mem, size_t len) {
  // A read with len=0 will create a future which can be used to signal
  // that a non-blocking socket has received data
  auto fd = ebbrt::root_vfs->Lookup(s);
  auto fdref = static_cast<ebbrt::EbbRef<ebbrt::SocketManager::SocketFd>>(fd);

  // return EAGAIN error for non-blocking sockets when no data is available
  if (len && (fdref->GetFlags() & O_NONBLOCK) && fdref->ReadWouldBlock()) {
    errno = EAGAIN;
    return -1;
  }
  auto fbuf = fdref->Read(len).Block();
  auto buf = std::move(fbuf.Get());
  auto chain_len = buf->ComputeChainDataLength();
  assert(chain_len <= len);
  if (chain_len > 0) {
    auto dptr = buf->GetDataPointer();
    std::memcpy(mem, dptr.Data(), chain_len);
    return chain_len;
  }
  {
    errno = EIO;
    return -1;
  }
}

int lwip_write(int s, const void* dataptr, size_t size) {
  auto fd = ebbrt::root_vfs->Lookup(s);
  auto buf = ebbrt::MakeUniqueIOBuf(size);
  std::memcpy(reinterpret_cast<void*>(buf->MutData()), dataptr, size);
  fd->Write(std::move(buf));
  return size;
}

int lwip_send(int s, const void* dataptr, size_t size, int flags) {
  return lwip_write(s, dataptr, size);
}

int lwip_recv(int s, void* mem, size_t len, int flags) {
  return lwip_read(s, mem, len);
}

int lwip_close(int s) {
  auto fd = ebbrt::root_vfs->Lookup(s);
  fd->Close().Block();
  return 0;
}

void lwip_assert(const char* fmt, ...) {
  EBBRT_UNIMPLEMENTED();
  return;
}

int lwip_shutdown(int s, int how) {
  EBBRT_UNIMPLEMENTED();
  return 0;
}

int lwip_getpeername(int s, struct sockaddr* name, socklen_t* namelen) {
  EBBRT_UNIMPLEMENTED();
  return 0;
}

int lwip_getsockname(int s, struct sockaddr* name, socklen_t* namelen) {
  EBBRT_UNIMPLEMENTED();
  return 0;
}

int lwip_getsockopt(int s, int level, int optname, void* optval,
                    socklen_t* optlen) {
  switch (level) {
  case SOL_SOCKET:
    switch (optname) {
    case SO_SNDBUF:
    case SO_RCVBUF:
      ebbrt::kprintf(
          "Warning(getsockopts): reporting fixed length of tcp s/r buffers\n");
      *(int*)optval = 29200;
      break;
    default:
      EBBRT_UNIMPLEMENTED();
    }
    break;
  default:
    EBBRT_UNIMPLEMENTED();
  }
  return 0;
}

int lwip_setsockopt(int s, int level, int optname, const void* optval,
                    socklen_t optlen) {

  switch (level) {
  case IPPROTO_TCP:
    switch (optname) {
    case TCP_NODELAY:
    case TCP_KEEPALIVE:
      break;
    default:
      //    case TCP_KEEPIDLE:
      //    case TCP_KEEPINTVL:
      //    case TCP_KEEPCNT:
      EBBRT_UNIMPLEMENTED();
    }
    break;
  case SOL_SOCKET:
    ebbrt::kprintf(
        "Warning(setsockopt): blind confirmation of SOL_SOCKET option #%d\n",
        optname);
    return 0;
  default:
    EBBRT_UNIMPLEMENTED();
  }
  return 0;
}

int lwip_recvfrom(int s, void* mem, size_t len, int flags,
                  struct sockaddr* from, socklen_t* fromlen) {
  EBBRT_UNIMPLEMENTED();
  return 0;
}

int lwip_sendto(int s, const void* dataptr, size_t size, int flags,
                const struct sockaddr* to, socklen_t tolen) {
  EBBRT_UNIMPLEMENTED();
  return 0;
}

int lwip_select(int maxfdp1, fd_set* readset, fd_set* writeset,
                fd_set* exceptset, struct timeval* timeout) {
  EBBRT_UNIMPLEMENTED();
  return 0;
}

int lwip_ioctl(int s, long cmd, void* argp) {
  EBBRT_UNIMPLEMENTED();
  return 0;
}

int lwip_fcntl(int s, int cmd, int val) {
  auto fd = ebbrt::root_vfs->Lookup(s);
  int f, newflag;
  switch (cmd) {
  case F_GETFL:
    f = (int)fd->GetFlags();
    return f;
    break;
  case F_SETFL:
    // File access mode (O_RDONLY, O_WRONLY, O_RDWR) and file creation
    // flags (i.e., O_CREAT, O_EXCL, O_NOCTTY, O_TRUNC) in arg  are  ignored. On
    // Linux this command can change only the O_APPEND, O_ASYNC, O_DIRECT,
    // O_NOATIME, and O_NONBLOCK flags.
    newflag = val & O_NONBLOCK;
    if (newflag) {
      auto f = fd->GetFlags();
      fd->SetFlags(f | newflag);
    } else {
      EBBRT_UNIMPLEMENTED();
    }
    break;
  default:
    EBBRT_UNIMPLEMENTED();
  };
  return 0;
}

int poll(struct pollfd* fds, nfds_t nfds, int timeout) {
  // TODO(jmc): support sockets across cores
  if (nfds > 1) {
    EBBRT_UNIMPLEMENTED();
  }
  for (uint32_t i = 0; i < nfds; ++i) {
    auto pfd = fds[i];
    if (pfd.fd < 0 || pfd.events == 0) {
      EBBRT_UNIMPLEMENTED();
      fds[i].revents = 0;
      break;
    }
    auto fd = static_cast<ebbrt::EbbRef<ebbrt::SocketManager::SocketFd>>(
        ebbrt::root_vfs->Lookup(pfd.fd));

    if ((pfd.events & POLLIN) && !fd->ReadWouldBlock()) {
      fds[i].revents |= POLLIN;
    }
    if ((pfd.events & POLLOUT) && !fd->WriteWouldBlock()) {
      fds[i].revents |= POLLOUT;
    }
    if ((~(POLLIN | POLLOUT) & pfd.events) != 0) {
      ebbrt::kabort("Poll event type unsupported: %x", pfd.events);
    }
  }
  return 0;
}

struct protoent* getprotobyname(const char* name) {
  if (strcmp(name, "tcp") || strcmp(name, "TCP")) {
    auto rtn = (struct protoent*)malloc(sizeof(struct protoent));
    rtn->p_name = const_cast<char*>(std::string("tcp").c_str());
    rtn->p_proto = IPPROTO_TCP;
    rtn->p_aliases = nullptr;
    return rtn;
  }
  return nullptr;
}

uint32_t htonl(uint32_t h) { return ebbrt::htonl(h); }

uint16_t htons(uint16_t h) { return ebbrt::htons(h); }

uint32_t ntohl(uint32_t h) { return ebbrt::ntohl(h); }

uint16_t ntohs(uint16_t h) { return ebbrt::ntohs(h); }

