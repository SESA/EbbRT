//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "VMemAllocator.h"

#include <cinttypes>
#include <mutex>

#include "../Align.h"
#include "Cpu.h"
#include "LocalIdMap.h"

namespace {
uintptr_t ReadCr2() {
  uintptr_t cr2;
  asm volatile("mov %%cr2, %[cr2]" : [cr2] "=r"(cr2));
  return cr2;
}
}

void ebbrt::VMemAllocator::Init() {
  auto rep = new VMemAllocator;
  local_id_map->Insert(
      std::make_pair(kVMemAllocatorId, boost::any(std::ref(*rep))));
}

ebbrt::VMemAllocator& ebbrt::VMemAllocator::HandleFault(EbbId id) {
  kassert(id == kVMemAllocatorId);
  LocalIdMap::ConstAccessor accessor;
  auto found = local_id_map->Find(accessor, id);
  kassert(found);
  auto ref =
      boost::any_cast<std::reference_wrapper<VMemAllocator>>(accessor->second);
  EbbRef<VMemAllocator>::CacheRef(id, ref.get());
  return ref;
}

ebbrt::VMemAllocator::VMemAllocator() {
  regions_.emplace(std::piecewise_construct,
                   std::forward_as_tuple(Pfn::Up(0xFFFF800000000000)),
                   std::forward_as_tuple(Pfn::Down(trans::kVMemStart)));
}

ebbrt::Pfn
ebbrt::VMemAllocator::Alloc(size_t npages,
                            std::unique_ptr<PageFaultHandler> pf_handler) {
  std::lock_guard<SpinLock> lock(lock_);
  for (auto it = regions_.begin(); it != regions_.end(); ++it) {
    const auto& begin = it->first;
    auto end = it->second.end();
    if (!it->second.IsFree() || end - begin < npages)
      continue;

    auto ret = end - npages;

    if (ret == begin) {
      it->second.set_page_fault_handler(std::move(pf_handler));
      it->second.set_allocated(true);
    } else {
      it->second.set_end(end - npages);
      auto p =
          regions_.emplace(std::piecewise_construct, std::forward_as_tuple(ret),
                           std::forward_as_tuple(ret + npages));
      kassert(p.second);
      p.first->second.set_page_fault_handler(std::move(pf_handler));
      p.first->second.set_allocated(true);
    }

    kprintf("Allocated %llx - %llx\n", ret.ToAddr(),
            (ret + npages).ToAddr() - 1);

    return ret;
  }
  kabort("%s: unable to allocate %llu virtual pages\n", __PRETTY_FUNCTION__,
         npages);
}

ebbrt::Pfn
ebbrt::VMemAllocator::Alloc(size_t npages, size_t pages_align,
                            std::unique_ptr<PageFaultHandler> pf_handler) {
  std::lock_guard<SpinLock> lock(lock_);
  for (auto it = regions_.begin(); it != regions_.end(); ++it) {
    const auto& begin = it->first;
    auto end = it->second.end();
    if (!it->second.IsFree() || end - begin < npages)
      continue;

    auto ret = Pfn(align::Down(end.val() - npages, pages_align));

    if (ret < begin)
      continue;

    auto endsize = (end - ret) - npages;

    if (ret == begin) {
      it->second.set_page_fault_handler(std::move(pf_handler));
      it->second.set_allocated(true);
    } else {
      it->second.set_end(ret);
      auto p =
          regions_.emplace(std::piecewise_construct, std::forward_as_tuple(ret),
                           std::forward_as_tuple(ret + npages));
      kassert(p.second);
      p.first->second.set_page_fault_handler(std::move(pf_handler));
      p.first->second.set_allocated(true);
    }

    if (endsize > 0) {
      regions_.emplace(std::piecewise_construct,
                       std::forward_as_tuple(ret + npages),
                       std::forward_as_tuple(end));
    }

    return ret;
  }
  kabort("%s: unable to allocate %llu virtual pages\n", __PRETTY_FUNCTION__,
         npages);
}

void ebbrt::VMemAllocator::HandlePageFault(idt::ExceptionFrame* ef) {
  std::lock_guard<SpinLock> lock(lock_);
  auto fault_addr = ReadCr2();

  if (fault_addr >= trans::kVMemStart) {
    trans::HandleFault(ef, fault_addr);
  } else {
    auto it = regions_.lower_bound(Pfn::Down(fault_addr));
    if (it == regions_.end() || it->second.end() < Pfn::Up(fault_addr)) {
      kprintf("Page fault for address %llx, no handler for it\n", fault_addr);
      ebbrt::kprintf("SS: %#018" PRIx64 " RSP: %#018" PRIx64 "\n", ef->ss,
                     ef->rsp);
      ebbrt::kprintf("FLAGS: %#018" PRIx64 "\n",
                     ef->rflags);  // TODO(Dschatz): print out actual meaning
      ebbrt::kprintf("CS: %#018" PRIx64 " RIP: %#018" PRIx64 "\n", ef->cs,
                     ef->rip);
      ebbrt::kprintf("Error Code: %" PRIx64 "\n", ef->error_code);
      ebbrt::kprintf("RAX: %#018" PRIx64 " RBX: %#018" PRIx64 "\n", ef->rax,
                     ef->rbx);
      ebbrt::kprintf("RCX: %#018" PRIx64 " RDX: %#018" PRIx64 "\n", ef->rcx,
                     ef->rdx);
      ebbrt::kprintf("RSI: %#018" PRIx64 " RDI: %#018" PRIx64 "\n", ef->rsi,
                     ef->rdi);
      ebbrt::kprintf("RBP: %#018" PRIx64 " R8:  %#018" PRIx64 "\n", ef->rbp,
                     ef->r8);
      ebbrt::kprintf("R9:  %#018" PRIx64 " R10: %#018" PRIx64 "\n", ef->r9,
                     ef->r10);
      ebbrt::kprintf("R11: %#018" PRIx64 " R12: %#018" PRIx64 "\n", ef->r11,
                     ef->r12);
      ebbrt::kprintf("R13: %#018" PRIx64 " R14: %#018" PRIx64 "\n", ef->r13,
                     ef->r14);
      ebbrt::kprintf("R15: %#018" PRIx64 "\n", ef->r15);
      // TODO(dschatz): FPU
      kabort();
    }
    it->second.HandleFault(ef, fault_addr);
  }
}

extern "C" void ebbrt::idt::PageFaultException(ExceptionFrame* ef) {
  vmem_allocator->HandlePageFault(ef);
}
