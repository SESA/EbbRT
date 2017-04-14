//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Debug.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/native/Net.h>
#include <ebbrt/native/NetTcpHandler.h>
#include <ebbrt/SharedIOBufRef.h>
#include <ebbrt/StaticSharedEbb.h>
#include <ebbrt/UniqueIOBuf.h>

namespace ebbrt {
class iPerf : public StaticSharedEbb<iPerf>, public CacheAligned {
public:
  iPerf(){};
  void Start(uint16_t port){
    listening_pcb_.Bind(port, [this](NetworkManager::TcpPcb pcb) {
      // round-robin connections to cores
      static std::atomic<size_t> cpu_index{0};
      auto index = cpu_index.fetch_add(1) % ebbrt::Cpu::Count();
      pcb.BindCpu(index);
      auto connection = new TcpSession(this, std::move(pcb));
      connection->Install();
    });
  }
  
private:
  class TcpSession : public ebbrt::TcpHandler {
  public:
    TcpSession(iPerf *iperf, ebbrt::NetworkManager::TcpPcb pcb)
      : ebbrt::TcpHandler(std::move(pcb)), iperf_(iperf) {}
    void Close(){}
    void Abort(){}
    void Receive(std::unique_ptr<MutIOBuf> b){
      return;
    }
  private:
    iPerf *iperf_;
  };
  NetworkManager::ListeningTcpPcb listening_pcb_;
};
}

void AppMain() {
  auto iperf = ebbrt::EbbRef<ebbrt::iPerf>(ebbrt::ebb_allocator->AllocateLocal());
  iperf->Start(5201);
  ebbrt::kprintf("iPerf server listening on port 5201\n");
}
