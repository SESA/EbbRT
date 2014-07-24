//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Debug.h>
#include <ebbrt/Net.h>
#include <ebbrt/IOBufRef.h>

ebbrt::NetworkManager::ListeningTcpPcb listening_pcb;
ebbrt::NetworkManager::TcpPcb connected_pcb;

class TcpHandler : public ebbrt::NetworkManager::ITcpHandler {
 public:
  explicit TcpHandler(ebbrt::NetworkManager::TcpPcb pcb)
      : pcb_(std::move(pcb)) {}

  void Receive(std::unique_ptr<ebbrt::MutIOBuf> buf) override {
    if (likely(buf)) {
      // echo back the data
      Send(std::move(buf));
    } else {
      // You may want to wait until the send queue is emptied to close
      ebbrt::kprintf("Close\n");
      pcb_.~TcpPcb();
    }
  }

  void WindowIncrease(uint16_t new_window) override {
    pcb_.SetWindowNotify(false);
    Send(std::move(buf_));
  }

  void Send(std::unique_ptr<ebbrt::IOBuf> buf) {
    // Do we have queued data?
    if (likely(!buf_)) {
      auto buf_len = buf->ComputeChainDataLength();
      auto window_len = pcb_.RemoteWindowRemaining();

      // Does the data length fit in the window?
      if (likely(buf_len <= window_len)) {
        pcb_.Send(std::move(buf));
        return;
      }

      // iterate over all buffers in the chain, at the end of this loop buf
      // should be null if the window is empty, otherwise enough data to fill
      // the window. The rest of the data should be in buf_
      for (auto& b : *buf) {
        auto len = buf->Length();
        if (len < window_len) {
          window_len -= len;
        } else {
          if (&b == buf.get()) {
            // The first buffer in the chain won't fit
            buf_ = std::move(buf);
            if (window_len > 0) {
              buf = ebbrt::CreateRef(*buf_);
              buf->TrimEnd(len - window_len);
              buf_->Advance(window_len);
            }
          } else {
            buf_ = buf->UnlinkEnd(b);
            if (window_len > 0) {
              auto ref = ebbrt::CreateRef(*buf_);
              ref->TrimEnd(len - window_len);
              buf->PrependChain(std::move(ref));
              buf_->Advance(window_len);
            }
            pcb_.Send(std::move(buf));
          }
          break;
        }
      }
      // If there was data to send, then do so
      if (buf)
        pcb_.Send(std::move(buf));
      pcb_.SetWindowNotify(true);
    } else {
      // queue buffer
      if (!buf_) {
        buf_ = std::move(buf);
      } else {
        buf_->PrependChain(std::move(buf));
      }
    }
  }

  void Install() { pcb_.InstallHandler(std::unique_ptr<ITcpHandler>(this)); }

 private:
  std::unique_ptr<ebbrt::IOBuf> buf_;
  ebbrt::NetworkManager::TcpPcb pcb_;
};

void AppMain() {
  listening_pcb.Bind(54321, [](ebbrt::NetworkManager::TcpPcb pcb) {
    ebbrt::kprintf("Connected\n");
    auto handler = new TcpHandler(std::move(pcb));
    handler->Install();
  });
}
