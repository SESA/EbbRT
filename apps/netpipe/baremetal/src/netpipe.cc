//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <cfloat>

#include <ebbrt/Clock.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/NetTcpHandler.h>
#include <ebbrt/StaticIOBuf.h>
#include <ebbrt/UniqueIOBuf.h>

#define TX

namespace {
const constexpr char sync_string[] = "SyncMe";
}

class NetPipeReceiver : public ebbrt::TcpHandler {
 public:
  explicit NetPipeReceiver(ebbrt::NetworkManager::TcpPcb pcb)
      : ebbrt::TcpHandler(std::move(pcb)) {}

  void Connected() override {
    ebbrt::event_manager->Spawn([this]() { DoTest(); },
                                /* force_async = */ true);
  }

  void DoTest() {
    size_t inc = (start > 1) ? start / 2 : 1;
    size_t nq = (start > 1) ? 1 : 0;
    size_t bufflen = start;

    struct data {
      double t;
      double bps;
      double variance;
      int bits;
      int repeat;
    };

    data bwdata[kNSamp];
    Sync();
    DoRPC(start, 100);
    Sync();
    auto tlast = t_ / 200.0;
    for (int n = 0, len = start; n < kNSamp - 3 && tlast < stoptm && len <= end;
         len = len + inc, nq++) {
      if (nq > 2)
        inc = ((nq % 2)) ? inc + inc : inc;

      for (int pert = ((perturbation > 0) && (inc > perturbation + 1))
                          ? -perturbation
                          : 0;
           pert <= perturbation;
           n++, pert += ((perturbation > 0) && (inc > perturbation + 1))
                            ? perturbation
                            : perturbation + 1) {
#ifdef TX
        size_t nrepeat = std::max(
            (runtm /
             ((static_cast<double>(bufflen) / (bufflen - inc + 1.0)) * tlast)),
            static_cast<double>(trials));
        SendRepeat(nrepeat);
#else
        size_t nrepeat = GetRepeat();
#endif
        bufflen = len + pert;
        bwdata[n].t = DBL_MAX;
#ifdef TX
        ebbrt::kprintf("%3d: %7d bytes %6d times --> ", n, bufflen, nrepeat);
#endif

        for (int i = 0; i < trials; ++i) {
          Sync();
          DoRPC(bufflen, nrepeat);
          t_ = t_ / static_cast<double>(nrepeat) / 2.0;
          bwdata[n].t = std::min(bwdata[n].t, t_);
        }

        if (bwdata[n].t == 0.0) {
          bwdata[n].t = 0.000001;
        }

        tlast = bwdata[n].t;
        bwdata[n].bits = bufflen * 8;
        bwdata[n].bps = bwdata[n].bits / (bwdata[n].t * 1024 * 1024);
        bwdata[n].repeat = nrepeat;
#ifdef TX
        // ebbrt::kprintf("%8d %lf %12.8lf\n", bwdata[n].bits / 8,
        // bwdata[n].bps,
        //                bwdata[n].t);
        ebbrt::kprintf(" %8.2lf Mbps in %10.2lf usec\n", bwdata[n].bps,
                       tlast * 1.0e6);
#endif
      }
    }
    Shutdown();
    ebbrt::kprintf("Netpipe complete\n");
  }

  void SendRepeat(size_t nrepeat) {
    auto buf = ebbrt::MakeUniqueIOBuf(sizeof(nrepeat));
    auto dp = buf->GetMutDataPointer();
    dp.Get<size_t>() = nrepeat;
    Send(std::move(buf));
    Pcb().Output();
  }

  size_t GetRepeat() {
    state_ = GET_REPEAT;
    ebbrt::event_manager->SaveContext(context_);
    return repeat_;
  }

  void Sync() {
    state_ = SYNC;
    auto buf = ebbrt::IOBuf::Create<ebbrt::StaticIOBuf>(sync_string);
    Send(std::move(buf));
    Pcb().Output();
    if (!synced_) {
      ebbrt::event_manager->SaveContext(context_);
    }
    synced_ = false;
  }

  void DoRPC(size_t buf_size, size_t iterations) {
    state_ = RPC;
    iterations_ = iterations;
    buf_size_ = buf_size;
#ifdef TX
    auto buf = ebbrt::MakeUniqueIOBuf(buf_size, /* zero_memory = */ true);
#endif
    t0_ = ebbrt::clock::Wall::Now();
#ifdef TX
    Send(std::move(buf));
    Pcb().Output();
#endif
    ebbrt::event_manager->SaveContext(context_);
    t_ = static_cast<double>((ebbrt::clock::Wall::Now() - t0_).count()) /
         1000000000.0;
  }

  // Callback to be invoked on a tcp segment receive event.
  void Receive(std::unique_ptr<ebbrt::MutIOBuf> buf) override {
    switch (state_) {
    case SYNC: {
      if (buf->ComputeChainDataLength() < strlen(sync_string)) {
        ebbrt::kabort("Didn't receive full sync string\n");
      }
      auto dp = buf->GetDataPointer();
      auto str = reinterpret_cast<const char*>(dp.Get(strlen(sync_string)));
      if (strncmp(sync_string, str, strlen(sync_string))) {
        ebbrt::kabort("Synchronization String Incorrect!\n");
      }
      ebbrt::event_manager->ActivateContext(std::move(context_));
      break;
    }
    case RPC: {
      if (buf_) {
        buf_->PrependChain(std::move(buf));
      } else {
        buf_ = std::move(buf);
      }
      auto chain_len = buf_->ComputeChainDataLength();
      if (chain_len >= buf_size_) {
        if (unlikely(chain_len > buf_size_)) {
          kassert(chain_len == (buf_size_ + strlen(sync_string)));
          // mark us as having received the sync and trim off that part
          synced_ = true;
          auto tail = buf_->Prev();
          auto to_trim = strlen(sync_string);
          while (to_trim > 0) {
            auto trim = std::max(tail->Length(), to_trim);
            tail->TrimEnd(trim);
            to_trim -= trim;
            tail = tail->Prev();
          }
        }
#ifndef TX
        Send(std::move(buf_));
#endif
        if (--iterations_ == 0) {
#ifdef TX
          buf_ = nullptr;
#endif
          ebbrt::event_manager->ActivateContext(std::move(context_));
#ifdef TX
        } else {
          Send(std::move(buf_));
#endif
        }
      }
      break;
    }
    case GET_REPEAT: {
      if (buf->ComputeChainDataLength() < sizeof(repeat_)) {
        ebbrt::kabort("Didn't receive full repeat\n");
      }
      auto dp = buf->GetDataPointer();
      repeat_ = dp.Get<size_t>();
      ebbrt::event_manager->ActivateContext(std::move(context_));
    }
    }
  }

  void Close() override { Shutdown(); }
  void Abort() override {}

 private:
  static const constexpr double runtm = 0.10;
  static const constexpr double stoptm = 1.0;
  static const constexpr int trials = 3;
  static const constexpr int kNSamp = 8000;
  static const constexpr size_t nbuff = 3;
  static const constexpr int perturbation = 0;
  static const constexpr size_t start = 1;
  static const constexpr int end = 1000000;
  enum states { SYNC, RPC, GET_REPEAT };
  enum states state_;
  std::unique_ptr<ebbrt::MutIOBuf> buf_;
  size_t iterations_;
  size_t buf_size_;
  ebbrt::EventManager::EventContext context_;
  ebbrt::clock::Wall::time_point t0_;
  double t_;
  size_t repeat_;
  bool synced_{false};
};

#ifdef TX
ebbrt::NetworkManager::TcpPcb pcb;
#else
ebbrt::NetworkManager::ListeningTcpPcb listening_pcb;
#endif

void AppMain() {
#ifdef TX
  pcb.Connect(ebbrt::Ipv4Address({10, 1, 81, 95}), 49152);
#else
  auto port = listening_pcb.Bind(0, [](ebbrt::NetworkManager::TcpPcb pcb) {
#endif
  auto handler = new NetPipeReceiver(std::move(pcb));
  handler->Install();
#ifndef TX
  handler->Connected();
});
ebbrt::kprintf("Listening on port %hu\n", port);
#endif
}
