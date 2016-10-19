//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_VMEMALLOCATOR_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_VMEMALLOCATOR_H_

#include <functional>
#include <map>
#include <memory>

#include "../CacheAligned.h"
#include "../SpinLock.h"
#include "Debug.h"
#include "EbbRef.h"
#include "Idt.h"
#include "Pfn.h"
#include "StaticIds.h"
#include "Trans.h"

namespace ebbrt {
class VMemAllocator : CacheAligned {
 public:
  class PageFaultHandler {
   public:
    virtual void HandleFault(idt::ExceptionFrame*, uintptr_t) = 0;
    virtual ~PageFaultHandler() {}
  };

  static void Init();
  static VMemAllocator& HandleFault(EbbId id);

  Pfn Alloc(size_t npages,
            std::unique_ptr<PageFaultHandler> pf_handler = nullptr);

  Pfn Alloc(size_t npages, size_t pages_align,
            std::unique_ptr<PageFaultHandler> pf_handler = nullptr);

 private:
  class Region {
   public:
    explicit Region(Pfn addr) : end_(addr), allocated_(false) {}
    bool IsFree() const { return !allocated_; }
    void HandleFault(idt::ExceptionFrame* ef, uintptr_t addr) {
      kbugon(IsFree(), "Fault on a free region!\n");
      page_fault_handler_->HandleFault(ef, addr);
    }
    Pfn end() const { return end_; }
    void set_end(Pfn end) { end_ = end; }
    void set_page_fault_handler(std::shared_ptr<PageFaultHandler> p) {
      page_fault_handler_ = std::move(p);
    }
    void set_allocated(bool allocated) { allocated_ = allocated; }

   private:
    Pfn end_;
    std::shared_ptr<PageFaultHandler> page_fault_handler_;
    bool allocated_;
  };

  VMemAllocator();
  void HandlePageFault(idt::ExceptionFrame* ef);

  SpinLock lock_;
  std::map<Pfn, Region, std::greater<Pfn>> regions_;

  friend void ebbrt::idt::PageFaultException(ExceptionFrame* ef);
};

constexpr auto vmem_allocator = EbbRef<VMemAllocator>(kVMemAllocatorId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_VMEMALLOCATOR_H_
