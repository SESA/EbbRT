//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Net.h>
#include <ebbrt/Debug.h>
#include <ebbrt/IOBufRef.h>

// A handler which implements the ITcpHandler interface for a
// connected tcp pcb. All callbacks are invoked on a single core.
class TcpHandler : public ebbrt::NetworkManager::ITcpHandler {
 public:
  // Constructor, just store the pcb
  explicit TcpHandler(ebbrt::NetworkManager::TcpPcb pcb)
      : pcb_(std::move(pcb)) {}

  // Closes and destroys the PCB when all pending data is sent
  void Shutdown() {
    if (buf_) {
      shutdown_ = true;
    } else {
      pcb_.~TcpPcb();
    }
  }

  // Callback to be invoked when the remote receive window has increased.
  void SendWindowIncrease() override {
    // Disable this callback
    pcb_.SetWindowNotify(false);
    pcb_.SetReceiveWindow(ebbrt::kTcpWnd);
    // Send any enqueued data
    Send(std::move(buf_));
    if (unlikely(shutdown_ && !buf_)) {
      pcb_.~TcpPcb();
    }
  }

  // Send data
  void Send(std::unique_ptr<ebbrt::IOBuf> buf) {
    kassert(buf);
    if (likely(!buf_)) {
      // no queued data
      auto buf_len = buf->ComputeChainDataLength();
      auto window_len = pcb_.SendWindowRemaining();

      // Does the data length fit in the window?
      if (likely(buf_len <= window_len)) {
        pcb_.Send(std::move(buf));
        return;
      }

      // iterate over all buffers in the chain, at the end of this
      // loop buf should be null if the window is empty, otherwise it
      // should hold enough data to fill the window. The rest of the
      // data should be in buf_
      for (auto& b : *buf) {
        auto len = b.Length();
        if (len < window_len) {
          // This buffer fits in the window, no truncation necessary
          window_len -= len;
        } else if (&b == buf.get()) {
          // The first buffer in the chain won't fit
          buf_ = std::move(buf);
          if (window_len > 0) {
            // If there is any space in the window, send what we can to buf to
            // be sent out and leave
            // the rest in buf_
            buf = ebbrt::CreateRef(*buf_);
            buf->TrimEnd(len - window_len);  // trim off what won't fit
            buf_->Advance(window_len);  // advance past what we will send out
          }
          break;
        } else {
          // A non-first buffer in the chain won't fit
          buf_ = buf->UnlinkEnd(b);  // remove the rest of the chain from buf
          if (window_len > 0) {
            // If there is any space in the window, append what we can to buf to
            // be sent out and leave the rest in buf_
            auto ref = ebbrt::CreateRef(*buf_);
            ref->TrimEnd(len - window_len);
            buf->PrependChain(std::move(ref));
            buf_->Advance(window_len);
          }
          break;
        }
      }
      // If there was data to send, then do so
      if (buf)
        pcb_.Send(std::move(buf));

      // There must be some data queued, so ask to be notified about a window
      // increase
      pcb_.SetWindowNotify(true);
      // Close the receive window to pace this connection
      pcb_.SetReceiveWindow(0);
    } else {
      // We already have data enqueued and to preserve ordering, that data must
      // go out first. In such a case, just enqueue the additional data
      buf_->PrependChain(std::move(buf));
    }
  }

  // Install ourselves as the handler for the pcb
  void Install() { pcb_.InstallHandler(std::unique_ptr<ITcpHandler>(this)); }

  void Connected() override {}

  ebbrt::NetworkManager::TcpPcb& Pcb() { return pcb_; }

 private:
  std::unique_ptr<ebbrt::IOBuf> buf_;
  ebbrt::NetworkManager::TcpPcb pcb_;
  bool shutdown_{false};
};
