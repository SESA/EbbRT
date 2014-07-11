//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NETETH_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NETETH_H_

#include <ebbrt/AtomicUniquePtr.h>
#include <ebbrt/NetIpAddress.h>
#include <ebbrt/Future.h>
#include <ebbrt/RcuList.h>

namespace ebbrt {
const constexpr size_t kEthHwAddrLen = 6;

typedef std::array<uint8_t, kEthHwAddrLen> EthernetAddress;

const constexpr EthernetAddress BroadcastMAC = {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

struct __attribute__((packed)) EthernetHeader {
  EthernetAddress dst;
  EthernetAddress src;
  uint16_t type;
};

const constexpr uint16_t kEthTypeIp = 0x800;
const constexpr uint16_t kEthTypeArp = 0x806;

struct __attribute__((packed)) ArpPacket {
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t oper;
  EthernetAddress sha;
  Ipv4Address spa;
  EthernetAddress tha;
  Ipv4Address tpa;
};

const constexpr uint16_t kHTypeEth = 1;
const constexpr uint16_t kPTypeIp = 0x800;

const constexpr uint16_t kArpRequest = 1;
const constexpr uint16_t kArpReply = 2;

class OperationQueue {
 public:
  class OperationBase {
   public:
    explicit OperationBase(OperationBase* next_) {
      next.store(next_, std::memory_order_relaxed);
    }
    virtual ~OperationBase() {}
    virtual void Invoke(const EthernetAddress& addr) = 0;

    std::atomic<OperationBase*> next;
  };

  template <typename F> class Operation : public OperationBase {
   public:
    Operation(F func, OperationBase* next)
        : OperationBase(next), func_(std::move(func)) {}

    void Invoke(const EthernetAddress& addr) override { func_(addr); }

    F func_;
  };

  OperationQueue() noexcept {
    queue_.store(nullptr, std::memory_order_release);
  }

  OperationQueue(OperationQueue&& other) noexcept {
    auto other_head = other.queue_.exchange(nullptr, std::memory_order_release);
    queue_.store(other_head, std::memory_order_release);
  }

  template <typename F> void Push(F&& func) {
    OperationBase* next;
    OperationBase* operation;
    do {
      next = queue_.load(std::memory_order_relaxed);
      operation = new Operation<F>(std::forward<F>(func), next);
    } while (!queue_.compare_exchange_weak(next, operation,
                                           std::memory_order_release,
                                           std::memory_order_relaxed));
  }

  void InvokeAndClear(const EthernetAddress& addr) {
    auto current = queue_.load(std::memory_order_consume);
    while (current != nullptr) {
      auto next = current->next.load(std::memory_order_consume);
      current->Invoke(addr);
      delete current;
      current = next;
    }
  }

  bool empty() const {
    auto front = queue_.load(std::memory_order_consume);
    return front == nullptr;
  }

 private:
  std::atomic<OperationBase*> queue_;
};

struct ArpEntry {
  ArpEntry(Ipv4Address paddr_, EthernetAddress* addr = nullptr)
      : paddr(paddr_), hwaddr(addr) {}

  void SetAddr(EthernetAddress* ptr) {
    auto old = hwaddr.exchange(ptr);
    // If there was no previous address, then we should check the queue to send
    // packets
    if (!old) {
      auto q = std::move(queue);
      q.InvokeAndClear(*ptr);
      event_manager->DoRcu(
          MoveBind([this, ptr](std::unique_ptr<EthernetAddress> old_addr) {
                     // We shouldn't have any modifications to the queue anymore
                     // so we can
                     // just clear it
                     queue.InvokeAndClear(*ptr);
                   },
                   std::move(old)));
    }
  }

  RcuHListHook hook;
  Ipv4Address paddr;
  atomic_unique_ptr<EthernetAddress> hwaddr{nullptr};
  OperationQueue queue;
};
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NETETH_H_
