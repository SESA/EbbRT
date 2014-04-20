//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_VIRTIO_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_VIRTIO_H_

#include <atomic>
#include <cstdint>
#include <limits>
#include <list>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ebbrt/Align.h>
#include <ebbrt/Buffer.h>
#include <ebbrt/Debug.h>
#include <ebbrt/Fls.h>
#include <ebbrt/PageAllocator.h>
#include <ebbrt/Pci.h>
#include <ebbrt/Pfn.h>

namespace ebbrt {

template <typename VirtType> class VirtioDriver {
 public:
  static bool Probe(pci::Device& dev) {
    if (dev.GetVendorId() == kVirtioVendorId &&
        dev.GetDeviceId() == VirtType::kDeviceId) {
      dev.DumpAddress();
      new VirtType(dev);
      return true;
    }
    return false;
  }

 protected:
  static const constexpr uint16_t kVirtioVendorId = 0x1AF4;

  static const constexpr size_t kDeviceFeatures = 0;
  static const constexpr size_t kGuestFeatures = 4;
  static const constexpr size_t kQueueAddress = 8;
  static const constexpr size_t kQueueSize = 12;
  static const constexpr size_t kQueueSelect = 14;
  static const constexpr size_t kQueueNotify = 16;
  static const constexpr size_t kDeviceStatus = 18;
  static const constexpr size_t kQueueVector = 22;
  static const constexpr size_t kDeviceConfiguration = 24;

  static const constexpr uint8_t kConfigAcknowledge = 1;
  static const constexpr uint8_t kConfigDriver = 2;
  static const constexpr uint8_t kConfigDriverOk = 4;

  static const constexpr int kQueueAddressShift = 12;

  static const constexpr int kVirtioRingEventIdx = 29;

  class VRing {
   public:
    VRing(VirtioDriver<VirtType>& driver, uint16_t qsize, size_t idx)
        : driver_(driver), idx_(idx), qsize_(qsize), free_head_(0),
          free_count_(qsize_), event_indexes_(false), interrupts_(true) {
      auto sz =
          align::Up(sizeof(Desc) * qsize + sizeof(uint16_t) * (3 + qsize),
                    4096) +
          align::Up(sizeof(uint16_t) * 3 + sizeof(UsedElem) * qsize, 4096);
      auto order = Fls(sz - 1) - pmem::kPageShift + 1;
      auto page = page_allocator->Alloc(order);
      kbugon(page == Pfn::None(), "virtio: page allocation failed");
      kprintf("VRING: %x\n", page.ToAddr());
      addr_ = reinterpret_cast<void*>(page.ToAddr());
      memset(addr_, 0, sz);

      desc_ = static_cast<Desc*>(addr_);
      avail_ = static_cast<Avail*>(static_cast<void*>(
          static_cast<char*>(addr_) + qsize_ * sizeof(Desc)));
      auto avail_ring_end = static_cast<void*>(&avail_->ring[qsize_]);
      used_event_ =
          reinterpret_cast<std::atomic<volatile uint16_t>*>(avail_ring_end);
      auto avail_end = static_cast<char*>(avail_ring_end) + sizeof(uint16_t);
      auto next_addr = align::Up(static_cast<void*>(avail_end), 4096);
      used_ = static_cast<Used*>(next_addr);
      auto used_ring_end = static_cast<void*>(&used_->ring[qsize_]);
      avail_event_ =
          reinterpret_cast<std::atomic<volatile uint16_t>*>(used_ring_end);

      for (unsigned i = 0; i < qsize_; ++i)
        desc_[i].next = i + 1;

      desc_[qsize_ - 1].next = 0;
    }

    void* addr() { return addr_; }

    size_t num_free_descriptors() { return free_count_; }

    template <typename Iterator>
    Iterator AddWritableBuffers(Iterator begin, Iterator end) {
      if (begin == end)
        return end;
      auto count = 0;
      auto orig_idx = avail_->idx.load(std::memory_order_relaxed);
      auto avail_idx = orig_idx;
      for (auto it = begin; it < end; ++it) {
        ++count;
        auto& buf_chain = *it;
        kassert(buf_chain->Unique());

        auto chain_len = buf_chain->CountChainElements();
        if (chain_len > free_count_)
          return it;

        // allocate the free descriptors
        free_count_ -= chain_len;
        uint16_t last_desc = free_head_;
        uint16_t head = free_head_;
        for (auto& buf : *buf_chain) {
          // for each buffer in this list, write it to a descriptor
          auto& desc = desc_[free_head_];
          desc.addr = reinterpret_cast<uint64_t>(buf.Data());
          desc.len = static_cast<uint32_t>(buf.Length());
          desc.flags |= Desc::Write | Desc::Next;
          last_desc = free_head_;
          free_head_ = desc.next;
        }
        // make sure the last descriptor is marked as such
        desc_[last_desc].flags &= ~Desc::Next;

        // add this descriptor chain to the avail ring
        avail_->ring[avail_idx % qsize_] = head;
        ++avail_idx;
        buf_references_.emplace(head, std::move(buf_chain));
      }
      // notify the device of the descriptor chains we added

      // ensure that all our writes to the descriptors and available ring have
      // completed
      std::atomic_thread_fence(std::memory_order_release);

      // give the device ownership of the added descriptor chains
      // note this need not have any memory ordering due to the preceding fence
      avail_->idx.store(avail_idx, std::memory_order_relaxed);

      // ensure that the previous write is seen before we detect if we must
      // notify the device. This ordering is to guarantee that the following
      // loads won't be ordered before the fence.
      std::atomic_thread_fence(std::memory_order_seq_cst);

      if (event_indexes_) {
        auto event_idx = avail_event_->load(std::memory_order_relaxed);
        if ((uint16_t)(avail_idx - event_idx - 1) <
            (uint16_t)(avail_idx - orig_idx)) {
          Kick();
        }
      } else if (!(used_->flags.load(std::memory_order_consume) &
                   Used::kNoNotify)) {
        Kick();
      }

      return end;
    }

    void AddReadableBuffer(std::unique_ptr<const IOBuf>&& bufs) {
      auto len = bufs->CountChainElements();
      kbugon(free_count_ < len,
             "Not enough free descriptors to add buffer chain\n");

      free_count_ -= len;
      uint16_t last_desc = free_head_;
      uint16_t head = free_head_;
      for (const auto& buf : *bufs) {
        auto addr = buf.Data();
        auto size = buf.Length();
        auto& desc = desc_[free_head_];
        desc.addr = reinterpret_cast<uint64_t>(addr);
        desc.len = size;
        desc.flags |= Desc::Next;
        last_desc = free_head_;
        free_head_ = desc.next;
      }
      desc_[last_desc].flags &= ~Desc::Next;

      auto avail_idx = avail_->idx.load(std::memory_order_relaxed);
      auto orig_idx = avail_idx;
      avail_->ring[avail_idx % qsize_] = head;
      ++avail_idx;

      std::atomic_thread_fence(std::memory_order_release);

      avail_->idx.store(avail_idx, std::memory_order_relaxed);

      std::atomic_thread_fence(std::memory_order_seq_cst);

      if (event_indexes_) {
        auto event_idx = avail_event_->load(std::memory_order_relaxed);
        if ((uint16_t)(avail_idx - event_idx - 1) <
            (uint16_t)(avail_idx - orig_idx)) {
          Kick();
        }
      } else if (!(used_->flags.load(std::memory_order_consume) &
                   Used::kNoNotify)) {
        Kick();
      }

      buf_references_.emplace(head, std::move(bufs));
    }

    template <typename F> void ProcessUsedBuffers(F&& f) {
      // future interrupts on this queue are implicitly disabled by our use of
      // the event index. We only get an interrupt when the new index crosses
      // the
      // event index.
      DisableInterrupts();
      auto used_index = last_used_;
      while (1) {
        if (used_index == used_->idx.load(std::memory_order_relaxed)) {
          // Ok we have processed all used buffers, set the event index to
          // re-enable interrupts
          last_used_ = used_index;
          if (event_indexes_) {
            used_event_->store(used_index, std::memory_order_relaxed);
          } else {
            EnableInterrupts();
          }

          // to avoid a race, we must double check after this barrier
          std::atomic_thread_fence(std::memory_order_seq_cst);

          if (used_index == used_->idx.load(std::memory_order_relaxed))
            break;

          DisableInterrupts();
          // otherwise the device did give us another used descriptor chain and
          // we must process it before enabling interrupts again.
        }
        auto& elem = used_->ring[used_index % qsize_];
        kassert(buf_references_.find(elem.id) != buf_references_.end());
        // This const cast is needed to trim the buffer chain
        auto buf = std::unique_ptr<IOBuf>(
            const_cast<IOBuf*>(buf_references_[elem.id].release()));
        buf_references_.erase(elem.id);
        Desc* descriptor = &desc_[elem.id];
        auto len = 1;
        while (descriptor->flags & Desc::Next) {
          ++len;
          descriptor = &desc_[descriptor->next];
        }
        descriptor->next = free_head_;
        free_head_ = elem.id;
        free_count_ += len;
        ++used_index;

        // trim the buffer chain to only include the actual size
        auto packet_len = elem.len;
        for (auto& b : *buf) {
          auto blen = b.Length();
          if (blen > packet_len) {
            b.TrimEnd(blen - packet_len);
            packet_len = 0;
          } else {
            packet_len -= blen;
          }
        }

        kassert(buf->ComputeChainDataLength() == elem.len);

        f(std::move(buf));
      }
    }

    uint16_t Size() const { return qsize_; }

    void Kick() { driver_.Kick(idx_); }

    void DisableAllInterrupts() {
      DisableInterrupts();
      interrupts_ = false;
    }

   private:
    struct Desc {
      static const constexpr uint16_t Next = 1;
      static const constexpr uint16_t Write = 2;
      static const constexpr uint16_t Indirect = 4;

      uint64_t addr;
      uint32_t len;
      uint16_t flags;
      uint16_t next;
    };

    struct Avail {
      static const constexpr uint16_t kNoInterrupt = 1;

      std::atomic<uint16_t> flags;
      std::atomic<volatile uint16_t> idx;
      uint16_t ring[];
    };

    struct UsedElem {
      uint32_t id;
      uint32_t len;
    };

    struct Used {
      static const constexpr uint16_t kNoNotify = 1;

      std::atomic<uint16_t> flags;
      std::atomic<volatile uint16_t> idx;
      UsedElem ring[];
    };

    void EnableInterrupts() {
      if (interrupts_) {
        auto avail_flags = avail_->flags.load(std::memory_order_relaxed);
        avail_->flags.store(avail_flags & ~Avail::kNoInterrupt,
                            std::memory_order_release);
      }
    }

    void DisableInterrupts() {
      if (interrupts_) {
        auto avail_flags = avail_->flags.load(std::memory_order_relaxed);
        avail_->flags.store(avail_flags | Avail::kNoInterrupt,
                            std::memory_order_release);
      }
    }

    VirtioDriver<VirtType>& driver_;
    size_t idx_;
    void* addr_;
    Desc* desc_;
    Avail* avail_;
    Used* used_;
    std::atomic<volatile uint16_t>* avail_event_;
    std::atomic<volatile uint16_t>* used_event_;
    uint16_t qsize_;
    uint16_t last_used_;
    uint16_t free_head_;
    uint16_t free_count_;
    std::unordered_map<uint16_t, std::unique_ptr<const IOBuf>> buf_references_;
    bool event_indexes_;
    bool interrupts_;
  };

  explicit VirtioDriver(pci::Device& dev) : dev_(dev), bar0_(dev.GetBar(0)) {
    dev_.SetBusMaster(true);
    auto msix = dev_.MsixEnable();
    kbugon(!msix, "Virtio without msix is unsupported\n");

    Reset();

    AddDeviceStatus(kConfigAcknowledge | kConfigDriver);

    SetupVirtQueues();

    SetupFeatures();
  }

  VRing& GetQueue(size_t index) { return queues_[index]; }

  void AddDeviceStatus(uint8_t status) {
    auto s = GetDeviceStatus();
    s |= status;
    SetDeviceStatus(s);
  }

  void Kick(size_t idx) { ConfigWrite16(kQueueNotify, idx); }

  uint8_t DeviceConfigRead8(size_t idx) {
    return ConfigRead8(kDeviceConfiguration + idx);
  }

  uint32_t GetDeviceFeatures() { return ConfigRead32(kDeviceFeatures); }

 private:
  uint8_t ConfigRead8(size_t offset) { return bar0_.Read8(offset); }

  uint16_t ConfigRead16(size_t offset) { return bar0_.Read16(offset); }

  uint32_t ConfigRead32(size_t offset) { return bar0_.Read32(offset); }

  void ConfigWrite8(size_t offset, uint8_t value) {
    bar0_.Write8(offset, value);
  }

  void ConfigWrite16(size_t offset, uint16_t value) {
    bar0_.Write16(offset, value);
  }

  void ConfigWrite32(size_t offset, uint32_t value) {
    bar0_.Write32(offset, value);
  }

  void Reset() { SetDeviceStatus(0); }

  uint8_t GetDeviceStatus() { return ConfigRead8(kDeviceStatus); }

  void SetDeviceStatus(uint8_t status) { ConfigWrite8(kDeviceStatus, status); }

  void SelectQueue(uint16_t queue) { ConfigWrite16(kQueueSelect, queue); }

  uint16_t GetQueueSize() { return ConfigRead16(kQueueSize); }

  void SetQueueAddr(void* addr) {
    auto addr_val = reinterpret_cast<uintptr_t>(addr);
    addr_val >>= kQueueAddressShift;
    kassert(addr_val <= std::numeric_limits<uint32_t>::max());
    ConfigWrite32(kQueueAddress, addr_val);
  }

  void SetQueueVector(uint16_t index) { ConfigWrite16(kQueueVector, index); }

  void SetupVirtQueues() {
    while (1) {
      auto idx = queues_.size();
      SelectQueue(idx);
      auto qsize = GetQueueSize();
      if (qsize == 0)
        return;

      queues_.emplace_back(*this, qsize, idx);
      SetQueueAddr(queues_.back().addr());
      SetQueueVector(idx);
    }
  }

  void SetGuestFeatures(uint32_t features) {
    ConfigWrite32(kGuestFeatures, features);
  }

  void SetupFeatures() {
    auto device_features = GetDeviceFeatures();
    kprintf("Device features: %x\n", device_features);
    auto driver_features = VirtType::GetDriverFeatures();

    // driver_features |= 1 << kVirtioRingEventIdx;

    auto subset = device_features & driver_features;

    // kassert(subset & 1 << kVirtioRingEventIdx);

    SetGuestFeatures(subset);
  }

  pci::Device& dev_;
  pci::Bar& bar0_;

  std::vector<VRing> queues_;
};
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_VIRTIO_H_
