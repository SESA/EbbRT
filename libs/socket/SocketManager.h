//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef SOCKETMANAGER_H_
#define SOCKETMANAGER_H_

#include <utility>  // std::pair

#include "Vfs.h"
#include <ebbrt/CacheAligned.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/Future.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Net.h>
#include <ebbrt/NetTcpHandler.h>
#include <ebbrt/SharedIOBufRef.h>
#include <ebbrt/SpinLock.h>
#include <ebbrt/StaticSharedEbb.h>
#include <ebbrt/UniqueIOBuf.h>

#include <sys/socket.h>

namespace ebbrt {

class SocketManager : public ebbrt::StaticSharedEbb<SocketManager>,
                      public CacheAligned {
 public:
  class SocketFd : public Vfs::Fd {
   public:
    class TcpSession : public ebbrt::TcpHandler, public ebbrt::Timer::Hook {
     public:
      void Fire() override;
      TcpSession(SocketFd* fd, ebbrt::NetworkManager::TcpPcb pcb)
          : ebbrt::TcpHandler(std::move(pcb)), fd_(fd), read_blocked_(false) {}
      void Connected() override;
      void Receive(std::unique_ptr<ebbrt::MutIOBuf> buf) override;
      void Close() override;
      void Abort() override;

     private:
      typedef std::pair<Promise<std::unique_ptr<ebbrt::IOBuf>>, size_t>
          PendingRead;
      friend class SocketFd;
      SocketFd* fd_;
      ebbrt::NetworkManager::TcpPcb pcb_;
      std::unique_ptr<ebbrt::MutIOBuf> inbuf_;
      ebbrt::SpinLock buf_lock_;
      PendingRead read_;
      Promise<uint8_t> disconnected_;
      Promise<uint8_t> connected_;
      bool read_blocked_;
      void check_read();
    };

    SocketFd() : listen_port_(0), flags_(0){};
    static EbbRef<SocketFd> Create(EbbId id = ebb_allocator->AllocateLocal()) {
      auto root = new SocketFd::Root();
      local_id_map->Insert(
          std::make_pair(id, static_cast<Vfs::Fd::Root*>(root)));
      return EbbRef<SocketFd>(id);
    }
    static SocketFd& HandleFault(EbbId id) {
      return static_cast<SocketFd&>(Vfs::Fd::HandleFault(id));
    }
    class Root : public Vfs::Fd::Root {
      std::atomic<SocketFd*> theRep;

     public:
      Root() : theRep(nullptr){};
      Vfs::Fd& HandleFault(EbbId id) override {
        if (!theRep) {
          auto tmp = new SocketFd();
          SocketFd* null = nullptr;
          if (!theRep.compare_exchange_strong(null, tmp)) {
            delete tmp;
          }
        }
        // Cache the reference to the rep in the local translation table
        EbbRef<SocketFd>::CacheRef(id, *theRep);
        return *theRep;
      }
    };

    int Listen();
    int Bind(uint16_t port);
    ebbrt::Future<int> Accept();
    ebbrt::Future<uint8_t> Connect(ebbrt::NetworkManager::TcpPcb pcb);
    // inherited
    ebbrt::Future<std::unique_ptr<IOBuf>> Read(size_t len) override;
    void Write(std::unique_ptr<IOBuf> buf) override;
    ebbrt::Future<uint8_t> Close() override;
    uint32_t GetFlags() override { return flags_; };
    void SetFlags(uint32_t f) override { flags_ = f; };

    // NONBLOCKING
    bool ReadWouldBlock();
    bool WriteWouldBlock() { return false; };

   private:
    void install_pcb(ebbrt::NetworkManager::TcpPcb pcb);
    bool is_nonblocking() { return flags_ & O_NONBLOCK; }
    void check_waiting();

    TcpSession* tcp_session_;
    ebbrt::NetworkManager::ListeningTcpPcb listening_pcb_;
    bool connected_;

    // listening tcp socket
    uint16_t listen_port_;
    std::queue<ebbrt::Promise<int>> waiting_accept_;
    std::queue<ebbrt::NetworkManager::TcpPcb> waiting_pcb_;
    ebbrt::SpinLock waiting_lock_;
    uint32_t flags_;
  };
  explicit SocketManager(){};
  int NewIpv4Socket();
};

//extern EbbId kSocketManagerEbbId;
static const auto socket_manager = EbbRef<SocketManager>();

}  // namespace ebbrt
#endif  // SOCKETMANAGER_H_
