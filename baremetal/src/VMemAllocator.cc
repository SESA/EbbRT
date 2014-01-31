//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/VMemAllocator.h>

#include <mutex>

#include <ebbrt/Cpu.h>
#include <ebbrt/LocalIdMap.h>

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
    } else {
      it->second.set_end(end - npages);
      auto p =
          regions_.emplace(std::piecewise_construct, std::forward_as_tuple(ret),
                           std::forward_as_tuple(ret + npages));
      kassert(p.second);
      p.first->second.set_page_fault_handler(std::move(pf_handler));
    }

    return ret;
  }
  kabort("%s: unable to allocate %llu virtual pages\n", __PRETTY_FUNCTION__,
         npages);
}

void ebbrt::VMemAllocator::HandlePageFault(idt::ExceptionFrame* ef) {
  std::lock_guard<SpinLock> lock(lock_);
  auto fault_addr = ReadCr2();
  auto it = regions_.lower_bound(Pfn::Down(fault_addr));
  kbugon(it == regions_.end() || it->second.end() < Pfn::Up(fault_addr),
         "Could not find region for faulting address!\n");
  it->second.HandleFault(ef, fault_addr);
}

extern "C" void ebbrt::idt::PageFaultException(ExceptionFrame* ef) {
  vmem_allocator->HandlePageFault(ef);
}
