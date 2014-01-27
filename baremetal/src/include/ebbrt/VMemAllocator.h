//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_VMEMALLOCATOR_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_VMEMALLOCATOR_H_

#include <functional>
#include <map>
#include <memory>

#include <ebbrt/CacheAligned.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/Idt.h>
#include <ebbrt/Pfn.h>
#include <ebbrt/SpinLock.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/Trans.h>

namespace ebbrt {
class VMemAllocator : CacheAligned {
 public:
  class PageFaultHandler {
   public:
    virtual void HandleFault(idt::ExceptionFrame *, uintptr_t) = 0;
    virtual ~PageFaultHandler() {}
  };

  static void Init();
  static VMemAllocator &HandleFault(EbbId id);

  Pfn Alloc(size_t npages,
            std::unique_ptr<PageFaultHandler> pf_handler = nullptr);

 private:
  class Region {
   public:
    explicit Region(Pfn addr) : end_(addr) {}
    bool IsFree() const { return !static_cast<bool>(page_fault_handler_); }
    void HandleFault(idt::ExceptionFrame *ef, uintptr_t addr) {
      kbugon(IsFree(), "Fault on a free region!\n");
      page_fault_handler_->HandleFault(ef, addr);
    }
    Pfn end() const { return end_; }
    void set_end(Pfn end) { end_ = end; }
    void set_page_fault_handler(std::shared_ptr<PageFaultHandler> p) {
      page_fault_handler_ = std::move(p);
    }

   private:
    Pfn end_;
    std::shared_ptr<PageFaultHandler> page_fault_handler_;
  };

  VMemAllocator();
  void HandlePageFault(idt::ExceptionFrame *ef);

  SpinLock lock_;
  std::map<Pfn, Region, std::greater<Pfn> > regions_;

  friend void ebbrt::idt::PageFaultException(ExceptionFrame *ef);
};

constexpr auto vmem_allocator = EbbRef<VMemAllocator>(kVMemAllocatorId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_VMEMALLOCATOR_H_
