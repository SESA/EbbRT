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

const constexpr uint16_t kArpRequest = 1;
const constexpr uint16_t kArpReply = 2;

class OperationQueue {
 public:
  class Operation {
   public:
    Operation(MovableFunction<void()> func, Operation* next) {
      func_ = std::move(func);
      next_.store(next, std::memory_order_relaxed);
    }

    std::atomic<Operation*> next_;
    MovableFunction<void()> func_;
  };

  template <typename Value>
  class iter : public boost::iterator_facade<iter<Value>, Value,
                                             boost::forward_traversal_tag> {
    struct enabler {};

   public:
    iter() : node_(nullptr) {}
    explicit iter(Operation* p) : node_(p) {}

    template <typename OtherValue>
    iter(
        const iter<OtherValue>& other,
        typename std::enable_if<std::is_convertible<OtherValue*, Value*>::value,
                                enabler>::type = enabler())
        : node_(other.node_) {}

   private:
    void increment() { node_ = node_->next_.load(std::memory_order_acquire); }

    template <typename OtherValue>
    bool equal(const iter<OtherValue>& other) const {
      return this->node_ == other.node_;
    }

    MovableFunction<void()>& dereference() const { return node_->func_; }

    Operation* node_;

    friend class boost::iterator_core_access;
  };

  typedef iter<MovableFunction<void()>> iterator;
  typedef iter<const MovableFunction<void()>> const_iterator;

  OperationQueue() noexcept {
    queue_.store(nullptr, std::memory_order_release);
  }

  OperationQueue(OperationQueue&& other) noexcept {
    auto other_head = other.queue_.exchange(nullptr, std::memory_order_release);
    queue_.store(other_head, std::memory_order_release);
  }

  void Push(MovableFunction<void()> func) {
    Operation* next;
    Operation* operation;
    do {
      next = queue_.load(std::memory_order_relaxed);
      operation = new Operation(std::move(func), next);
    } while (queue_.compare_exchange_weak(
        next, operation, std::memory_order_release, std::memory_order_relaxed));
  }

  iterator begin() { return iterator(queue_.load(std::memory_order_consume)); }
  iterator end() { return iterator(nullptr); }

  const_iterator cbegin() const {
    return const_iterator(queue_.load(std::memory_order_consume));
  }
  const_iterator cend() const { return const_iterator(nullptr); }

  bool empty() const {
    auto front = queue_.load(std::memory_order_consume);
    return front == nullptr;
  }

 private:
  std::atomic<Operation*> queue_;
};

struct ArpEntry {
  ArpEntry(Ipv4Address paddr_, const EthernetAddress& addr)
      : paddr(paddr_), hwaddr(new EthernetAddress(addr)) {}

  void SetAddr(const EthernetAddress& addr) {
    auto ptr = new EthernetAddress(addr);
    auto old = hwaddr.exchange(ptr);
    // If there was no previous address, then we should check the queue to send
    // packets
    if (!old) {
      auto q = std::move(queue);
      kbugon(!q.empty(), "Send out queued packets\n");
      event_manager->DoRcu([this]() {
        auto q = std::move(queue);
        kbugon(!q.empty(), "Send out queued packets\n");
      });
    }
  }

  RcuHListHook hook;
  Ipv4Address paddr;
  atomic_unique_ptr<EthernetAddress> hwaddr{nullptr};
  OperationQueue queue;
};
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NETETH_H_
