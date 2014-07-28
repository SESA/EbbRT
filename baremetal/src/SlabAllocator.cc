//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/SlabAllocator.h>

#include <mutex>

#include <boost/container/static_vector.hpp>

#include <ebbrt/Debug.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/ExplicitlyConstructed.h>
#include <ebbrt/Fls.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/PageAllocator.h>

namespace {
size_t SlabOrder(size_t size, size_t max_order, unsigned frac) {
  size_t order;
  if (ebbrt::Fls(size - 1) < ebbrt::pmem::kPageShift)
    order = 0;
  else
    order = ebbrt::Fls(size - 1) - ebbrt::pmem::kPageShift + 1;

  while (order <= max_order) {
    auto slab_size = ebbrt::pmem::kPageSize << order;
    auto objects = slab_size / size;
    auto waste = slab_size - (objects * size);

    if (waste * frac <= slab_size)
      break;

    ++order;
  }

  return order;
}

// These numbers come from the defunct SLQB allocator in Linux, may need tuning
size_t CalculateOrder(size_t size) {
  auto order = SlabOrder(size, 1, 4);
  if (order <= 1)
    return order;

  order = SlabOrder(size, ebbrt::PageAllocator::kMaxOrder, 0);
  ebbrt::kbugon(order > ebbrt::PageAllocator::kMaxOrder,
                "Request for too big a slab\n");
  return order;
}

size_t CalculateFreebatch(size_t size) {
  return std::max(4 * ebbrt::pmem::kPageSize / size,
                  std::min(size_t(256), 64 * ebbrt::pmem::kPageSize / size));
}

ebbrt::Pfn AddrToSlabPfn(void* addr, size_t order) {
  auto pfn = ebbrt::Pfn::Down(addr);
  return ebbrt::Pfn(ebbrt::align::Down(pfn.val(), 1 << order));
}

ebbrt::ExplicitlyConstructed<ebbrt::SlabAllocatorRoot> slab_root_allocator;
boost::container::static_vector<ebbrt::SlabAllocatorNode,
                                ebbrt::numa::kMaxNodes>
slab_root_allocator_node_allocators;
boost::container::static_vector<ebbrt::SlabAllocator, ebbrt::Cpu::kMaxCpus>
slab_root_allocator_cpu_allocators;

ebbrt::ExplicitlyConstructed<ebbrt::SlabAllocatorRoot> slab_node_allocator;
boost::container::static_vector<ebbrt::SlabAllocatorNode,
                                ebbrt::numa::kMaxNodes>
slab_node_allocator_node_allocators;
boost::container::static_vector<ebbrt::SlabAllocator, ebbrt::Cpu::kMaxCpus>
slab_node_allocator_cpu_allocators;

ebbrt::ExplicitlyConstructed<ebbrt::SlabAllocatorRoot> slab_cpu_allocator;
boost::container::static_vector<ebbrt::SlabAllocatorNode,
                                ebbrt::numa::kMaxNodes>
slab_cpu_allocator_node_allocators;
boost::container::static_vector<ebbrt::SlabAllocator, ebbrt::Cpu::kMaxCpus>
slab_cpu_allocator_cpu_allocators;
}  // namespace

void ebbrt::slab::Init() {
  slab_root_allocator.construct(sizeof(SlabAllocatorRoot),
                                alignof(SlabAllocatorRoot));
  slab_node_allocator.construct(sizeof(SlabAllocatorNode),
                                alignof(SlabAllocatorNode));
  slab_cpu_allocator.construct(sizeof(SlabAllocator), alignof(SlabAllocator));

  for (unsigned i = 0; i < numa::nodes.size(); ++i) {
    slab_root_allocator_node_allocators.emplace_back(*slab_root_allocator,
                                                     Nid(i));
    slab_root_allocator->SetNodeAllocator(
        &slab_root_allocator_node_allocators[i], Nid(i));

    slab_node_allocator_node_allocators.emplace_back(*slab_node_allocator,
                                                     Nid(i));
    slab_node_allocator->SetNodeAllocator(
        &slab_node_allocator_node_allocators[i], Nid(i));

    slab_cpu_allocator_node_allocators.emplace_back(*slab_cpu_allocator,
                                                    Nid(i));
    slab_cpu_allocator->SetNodeAllocator(&slab_cpu_allocator_node_allocators[i],
                                         Nid(i));
  }

  for (unsigned i = 0; i < Cpu::Count(); ++i) {
    slab_root_allocator_cpu_allocators.emplace_back(*slab_root_allocator);
    slab_root_allocator->SetCpuAllocator(
        std::unique_ptr<SlabAllocator>(&slab_root_allocator_cpu_allocators[i]),
        i);

    slab_node_allocator_cpu_allocators.emplace_back(*slab_cpu_allocator);
    slab_node_allocator->SetCpuAllocator(
        std::unique_ptr<SlabAllocator>(&slab_node_allocator_cpu_allocators[i]),
        i);

    slab_cpu_allocator_cpu_allocators.emplace_back(*slab_cpu_allocator);
    slab_cpu_allocator->SetCpuAllocator(
        std::unique_ptr<SlabAllocator>(&slab_cpu_allocator_cpu_allocators[i]),
        i);
  }
}

ebbrt::SlabCache::SlabCache(SlabAllocatorRoot& root)
    : root_(root), remote_check(false) {}
ebbrt::SlabCache::~SlabCache() {
  kassert(object_list_.empty());
  kassert(remote_.list.empty());

  if (!partial_page_list_.empty()) {
    kprintf("Memory leak detected, slab partial page list is not empty\n");
  }
}

void* ebbrt::SlabCache::Alloc() {
retry:
  // first check the free list and return if we have a free object
  if (!object_list_.empty()) {
    auto& ret = object_list_.front();
    object_list_.pop_front();
    return ret.addr();
  }

  // no free object in lock list, potentially check the list of remotely freed
  // objects
  if (remote_check) {
    ClaimRemoteFreeList();

    while (object_list_.size() > root_.hiwater())
      FlushFreeList(root_.free_batch());

    goto retry;
  }

  // no free object in either list, try to get an object from a partial page
  if (!partial_page_list_.empty()) {
    auto& page = partial_page_list_.front();
    auto& page_slab_data = page.data.slab_data;
    kassert(page_slab_data.used < root_.NumObjectsPerSlab());
    // if this is the last allocation remove it from the list
    if (page_slab_data.used + 1 == root_.NumObjectsPerSlab()) {
      partial_page_list_.pop_front();
    }

    kassert(!page_slab_data.list->empty());

    ++page_slab_data.used;

    auto& object = page_slab_data.list->front();
    page_slab_data.list->pop_front();
    return object.addr();
  }

  return nullptr;
}

void ebbrt::SlabCache::AddSlab(Pfn pfn) {
  auto page = mem_map::PfnToPage(pfn);
  kassert(page != nullptr);

  page->usage = mem_map::Page::Usage::kSlabAllocator;

  auto& page_slab_data = page->data.slab_data;
  page_slab_data.member_hook.construct();
  page_slab_data.list.construct();
  page_slab_data.used = 0;

  // Initialize free list
  auto start = pfn.ToAddr();
  auto end = pfn.ToAddr() + root_.NumObjectsPerSlab() * root_.size();
  for (uintptr_t addr = start; addr < end; addr += root_.size()) {
    // add object to per slab free list
    auto object = new (reinterpret_cast<void*>(addr)) FreeObject();
    page_slab_data.list->push_front(*object);

    auto addr_page = mem_map::AddrToPage(addr);
    kassert(addr_page != nullptr);
    auto& addr_page_slab_data = addr_page->data.slab_data;
    addr_page_slab_data.cache = this;
  }

  partial_page_list_.push_front(*page);
}

void ebbrt::SlabCache::Free(void* p) {
  auto object = new (p) FreeObject();
  object_list_.push_front(*object);
  if (object_list_.size() > root_.hiwater()) {
    FlushFreeList(root_.free_batch());
  }
}

void ebbrt::SlabCache::FlushFreeList(size_t amount) {
  size_t freed = 0;
  while (!object_list_.empty() && freed < amount) {
    auto& object = object_list_.front();
    object_list_.pop_front();
    auto obj_addr = object.addr();
    auto pfn = AddrToSlabPfn(obj_addr, root_.order());
    auto page = mem_map::PfnToPage(pfn);
    kassert(page != nullptr);

    auto& page_slab_data = page->data.slab_data;

    if (page_slab_data.cache != this) {
      auto& allocator = root_.GetCpuAllocator();
      allocator.FreeRemote(obj_addr);
    } else {
      auto object = new (obj_addr) FreeObject();
      page_slab_data.list->push_front(*object);
      --page_slab_data.used;

      if (page_slab_data.used == 0) {
        if (root_.NumObjectsPerSlab() > 1) {
          partial_page_list_.erase(partial_page_list_.iterator_to(*page));
        }

        // free the page
        page_slab_data.member_hook.destruct();
        page_slab_data.list.destruct();
        page_allocator->Free(pfn, root_.order());
      } else if (page_slab_data.used + 1 == root_.NumObjectsPerSlab()) {
        partial_page_list_.push_front(*page);
      }
    }

    ++freed;
  }
}

void ebbrt::SlabCache::FlushFreeListAll() { FlushFreeList(size_t(-1)); }

void ebbrt::SlabCache::ClaimRemoteFreeList() {
  std::lock_guard<SpinLock> lock(remote_.lock);

  if (object_list_.empty()) {
    object_list_.swap(remote_.list);
  } else {
    object_list_.splice_after(object_list_.end(), remote_.list);
  }
}

ebbrt::EbbRef<ebbrt::SlabAllocator>
ebbrt::SlabAllocator::Construct(size_t size) {
  auto id = ebb_allocator->AllocateLocal();
  auto allocator_root = new SlabAllocatorRoot(size);
  local_id_map->Insert(std::make_pair(id, allocator_root));
  return EbbRef<ebbrt::SlabAllocator>{id};
}

ebbrt::SlabAllocator& ebbrt::SlabAllocator::HandleFault(EbbId id) {
  SlabAllocatorRoot* allocator_root;
  {
    LocalIdMap::ConstAccessor accessor;
    auto found = local_id_map->Find(accessor, id);
    kassert(found);
    allocator_root = boost::any_cast<SlabAllocatorRoot*>(accessor->second);
  }
  kassert(allocator_root != nullptr);
  auto& allocator = allocator_root->GetCpuAllocator();
  EbbRef<SlabAllocator>::CacheRef(id, allocator);
  return allocator;
}

ebbrt::SlabAllocator::SlabAllocator(SlabAllocatorRoot& root)
    : cache_(root), remote_cache_(nullptr) {}

void* ebbrt::SlabAllocator::operator new(size_t size, Nid nid) {
  kassert(size == sizeof(SlabAllocator));
  if (nid == Cpu::GetMyNode()) {
    auto& allocator = slab_cpu_allocator->GetCpuAllocator();
    auto ret = allocator.Alloc();
    if (ret == nullptr) {
      throw std::bad_alloc();
    }
    return ret;
  }

  auto& allocator = slab_cpu_allocator->GetNodeAllocator(nid);
  auto ret = allocator.Alloc();
  if (ret == nullptr) {
    throw std::bad_alloc();
  }
  return ret;
}

void ebbrt::SlabAllocator::operator delete(void* p) {
  auto& allocator = slab_cpu_allocator->GetCpuAllocator();
  allocator.Free(p);
}

void* ebbrt::SlabAllocator::Alloc() {
  auto ret = cache_.Alloc();
  if (unlikely(ret == nullptr)) {
    auto pfn = page_allocator->Alloc(cache_.root_.order(), Cpu::GetMyNode());
    if (pfn == Pfn::None())
      return nullptr;
    cache_.AddSlab(pfn);
    ret = cache_.Alloc();
    kassert(ret != nullptr);
  }
  return ret;
}

void* ebbrt::SlabAllocator::Alloc(Nid nid) {
  if (nid == Cpu::GetMyNode()) {
    return Alloc();
  }

  auto& node_allocator = cache_.root_.GetNodeAllocator(nid);
  return node_allocator.Alloc();
}

void ebbrt::SlabAllocator::Free(void* p) {
  auto page = mem_map::AddrToPage(p);
  kassert(page != nullptr);

  auto nid = page->nid;
  if (Nid(nid) == Cpu::GetMyNode()) {
    cache_.Free(p);
  } else {
    FreeRemote(p);
  }
}

void ebbrt::SlabAllocator::FreeRemote(void* p) {
  auto page = mem_map::AddrToPage(p);
  kassert(page != nullptr);

  auto& page_slab_data = page->data.slab_data;
  if (page_slab_data.cache != remote_cache_) {
    FlushRemoteList();
    remote_cache_ = page_slab_data.cache;
  }

  auto object = new (p) FreeObject;
  remote_list_.push_front(*object);

  if (remote_list_.size() > cache_.root_.free_batch()) {
    FlushRemoteList();
  }
}

void ebbrt::SlabAllocator::FlushRemoteList() {
  if (remote_cache_ == nullptr)
    return;

  std::lock_guard<SpinLock> lock(remote_cache_->remote_.lock);
  auto& list = remote_cache_->remote_.list;
  auto size = list.size();
  if (list.empty()) {
    list.swap(remote_list_);
  } else {
    list.splice_after(list.begin(), remote_list_);
  }
  auto flush_watermark = cache_.root_.free_batch();
  if (size < flush_watermark && list.size() >= flush_watermark) {
    // We crossed the watermark on this flush, mark the remote_check
    remote_cache_->remote_check = true;
  }
}

ebbrt::SlabAllocatorNode::SlabAllocatorNode(SlabAllocatorRoot& root, Nid nid)
    : cache_(root), nid_(nid) {}

void* ebbrt::SlabAllocatorNode::operator new(size_t size, Nid nid) {
  kassert(size == sizeof(SlabAllocatorNode));
  if (nid == Cpu::GetMyNode()) {
    auto& allocator = slab_node_allocator->GetCpuAllocator();
    auto ret = allocator.Alloc();
    if (ret == nullptr) {
      throw std::bad_alloc();
    }
    return ret;
  }

  auto& allocator = slab_node_allocator->GetNodeAllocator(nid);
  auto ret = allocator.Alloc();
  if (ret == nullptr) {
    throw std::bad_alloc();
  }
  return ret;
}

void ebbrt::SlabAllocatorNode::operator delete(void* p) {
  auto& allocator = slab_node_allocator->GetCpuAllocator();
  allocator.Free(p);
}

void* ebbrt::SlabAllocatorNode::Alloc() {
  std::lock_guard<SpinLock> lock(lock_);
  auto ret = cache_.Alloc();
  if (ret == nullptr) {
    auto pfn = page_allocator->Alloc(cache_.root_.order(), nid_);
    if (pfn == Pfn::None())
      return nullptr;
    cache_.AddSlab(pfn);
    ret = cache_.Alloc();
    kassert(ret != nullptr);
  }
  return ret;
}

ebbrt::SlabAllocatorRoot::SlabAllocatorRoot(size_t size_in, size_t align_in)
    : align_(align::Up(std::max(align_in, sizeof(void*)), sizeof(void*))),
      size_(align::Up(std::max(size_in, sizeof(void*)), align_)),
      order_(CalculateOrder(size_)), free_batch_(CalculateFreebatch(size_)),
      hiwater_(free_batch_ * 4) {
  std::fill(node_allocators_.begin(), node_allocators_.end(), nullptr);
  std::fill(cpu_allocators_.begin(), cpu_allocators_.end(), nullptr);
}

ebbrt::SlabAllocatorRoot::~SlabAllocatorRoot() {
  for (auto& cpu_allocator : cpu_allocators_) {
    auto allocator = cpu_allocator.get();
    if (allocator != nullptr) {
      allocator->cache_.FlushFreeListAll();
      allocator->FlushRemoteList();
    }
  }

  for (auto& cpu_allocator : cpu_allocators_) {
    auto allocator = cpu_allocator.get();
    if (allocator != nullptr) {
      allocator->cache_.ClaimRemoteFreeList();
      allocator->cache_.FlushFreeListAll();
    }
  }

  for (auto& node_allocator : node_allocators_) {
    auto allocator = node_allocator.load();
    if (allocator != nullptr) {
      allocator->cache_.ClaimRemoteFreeList();
      allocator->cache_.FlushFreeListAll();
      delete (allocator);
    }
  }
}

void* ebbrt::SlabAllocatorRoot::operator new(size_t size) {
  kassert(size == sizeof(SlabAllocatorRoot));
  auto& allocator = slab_root_allocator->GetCpuAllocator();
  auto ret = allocator.Alloc();
  if (ret == nullptr) {
    throw std::bad_alloc();
  }
  return ret;
}

void ebbrt::SlabAllocatorRoot::operator delete(void* p) {
  auto& allocator = slab_root_allocator->GetCpuAllocator();
  allocator.Free(p);
}

size_t ebbrt::SlabAllocatorRoot::NumObjectsPerSlab() {
  return (pmem::kPageSize << order_) / size_;
}

void ebbrt::SlabAllocatorRoot::SetCpuAllocator(std::unique_ptr<SlabAllocator> a,
                                               size_t cpu_index) {
  cpu_allocators_[cpu_index] = std::move(a);
}

void ebbrt::SlabAllocatorRoot::SetNodeAllocator(SlabAllocatorNode* a, Nid nid) {
  node_allocators_[nid.val()] = a;
}

ebbrt::SlabAllocator&
ebbrt::SlabAllocatorRoot::GetCpuAllocator(size_t cpu_index) {
  auto allocator = cpu_allocators_[cpu_index].get();
  if (allocator == nullptr) {
    allocator = new (Cpu::GetByIndex(cpu_index)->nid()) SlabAllocator(*this);
    cpu_allocators_[cpu_index] = std::unique_ptr<SlabAllocator>(allocator);
  }
  return *allocator;
}

ebbrt::SlabAllocatorNode& ebbrt::SlabAllocatorRoot::GetNodeAllocator(Nid nid) {
  size_t index = nid.val();
  auto allocator = node_allocators_[index].load();
  if (allocator == nullptr) {
    auto new_allocator = new (nid) SlabAllocatorNode(*this, nid);
    if (!node_allocators_[index].compare_exchange_strong(allocator,
                                                         new_allocator)) {
      delete allocator;
      allocator = node_allocators_[index].load();
    } else {
      allocator = new_allocator;
    }
  }
  return *allocator;
}
