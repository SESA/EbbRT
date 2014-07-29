//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_SLABALLOCATOR_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_SLABALLOCATOR_H_

#include <atomic>
#include <memory>

#include <boost/intrusive/parent_from_member.hpp>

#include <ebbrt/Align.h>
#include <ebbrt/CacheAligned.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/MemMap.h>
#include <ebbrt/Numa.h>
#include <ebbrt/PageAllocator.h>
#include <ebbrt/SlabObject.h>
#include <ebbrt/SpinLock.h>
#include <ebbrt/Trans.h>

namespace ebbrt {

namespace slab {
void Init();
}

struct SlabAllocatorRoot;

class SlabCache {
 public:
  struct Remote : public CacheAligned {
    SpinLock lock;
    FreeObjectList list;
  } remote_;

  explicit SlabCache(SlabAllocatorRoot& root);
  ~SlabCache();

  void* Alloc();
  void Free(void* p);
  void AddSlab(Pfn pfn);
  void FlushFreeList(size_t amount);
  void FlushFreeListAll();
  void ClaimRemoteFreeList();

  SlabAllocatorRoot& root_;
  std::atomic<bool> remote_check;

 private:
  struct PageHookFunctor {
    typedef boost::intrusive::list_member_hook<
        boost::intrusive::link_mode<boost::intrusive::normal_link>> hook_type;
    typedef hook_type* hook_ptr;
    typedef const hook_type* const_hook_ptr;
    typedef mem_map::Page value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;

    static hook_ptr to_hook_ptr(value_type& value) {
      return &(*value.data.slab_data.member_hook);
    }

    static const_hook_ptr to_hook_ptr(const value_type& value) {
      return &(*value.data.slab_data.member_hook);
    }

    static pointer to_value_ptr(hook_ptr n) {
      auto explicit_ptr =
          static_cast<ExplicitlyConstructed<hook_type>*>(static_cast<void*>(n));
      auto slab_data_ptr = boost::intrusive::get_parent_from_member(
          explicit_ptr, &mem_map::Page::Data::SlabData::member_hook);
      auto data_ptr = boost::intrusive::get_parent_from_member(
          slab_data_ptr, &mem_map::Page::Data::slab_data);
      return boost::intrusive::get_parent_from_member(data_ptr,
                                                      &mem_map::Page::data);
    }

    static const_pointer to_value_ptr(const_hook_ptr n) {
      auto explicit_ptr = static_cast<const ExplicitlyConstructed<hook_type>*>(
          static_cast<const void*>(n));
      auto slab_data_ptr = boost::intrusive::get_parent_from_member(
          explicit_ptr, &mem_map::Page::Data::SlabData::member_hook);
      auto data_ptr = boost::intrusive::get_parent_from_member(
          slab_data_ptr, &mem_map::Page::Data::slab_data);
      return boost::intrusive::get_parent_from_member(data_ptr,
                                                      &mem_map::Page::data);
    }
  };

  typedef boost::intrusive::list<  // NOLINT
      mem_map::Page, boost::intrusive::function_hook<PageHookFunctor>> PageList;

  FreeObjectList object_list_;
  PageList partial_page_list_;
};

class SlabAllocator : public CacheAligned {
 public:
  static const constexpr size_t kMaxSlabSize =
      1 << (pmem::kPageShift + PageAllocator::kMaxOrder);

  explicit SlabAllocator(SlabAllocatorRoot& root);

  static EbbRef<SlabAllocator> Construct(size_t size);
  static SlabAllocator& HandleFault(EbbId id);
  void* operator new(size_t size, Nid nid);
  void operator delete(void* p);

  void* Alloc();
  void* Alloc(Nid nid);
  void Free(void* p);

 private:
  void FreeRemote(void* p);
  void FlushRemoteList();

  SlabCache cache_;
  FreeObjectList remote_list_;
  SlabCache* remote_cache_;

  friend class SlabCache;
  friend class SlabAllocatorRoot;
};

class SlabAllocatorNode : public CacheAligned {
 public:
  SlabAllocatorNode(SlabAllocatorRoot& root, Nid nid);

 private:
  void* Alloc();
  void* operator new(size_t size, Nid nid);
  void operator delete(void* p);

  SlabCache cache_;
  Nid nid_;
  SpinLock lock_;

  friend class SlabAllocator;
  friend class SlabAllocatorRoot;
};

class SlabAllocatorRoot {
 public:
  SlabAllocatorRoot(size_t size, size_t align = 0);
  ~SlabAllocatorRoot();

  void* operator new(size_t size);
  void operator delete(void* p);
  size_t NumObjectsPerSlab();
  SlabAllocator& GetCpuAllocator(size_t cpu_index = Cpu::GetMine());
  SlabAllocatorNode& GetNodeAllocator(Nid nid);
  void SetCpuAllocator(std::unique_ptr<SlabAllocator>, size_t cpu_index);
  void SetNodeAllocator(SlabAllocatorNode*, Nid nid);
  size_t size() const { return size_; }
  size_t order() const { return order_; }
  size_t free_batch() const { return free_batch_; }
  size_t hiwater() const { return hiwater_; }

 private:
  size_t align_;
  size_t size_;
  size_t order_;
  size_t free_batch_;
  size_t hiwater_;
  // TODO(dschatz): atomic_unique_ptr?
  std::array<std::atomic<SlabAllocatorNode*>, numa::kMaxNodes> node_allocators_;
  std::array<std::unique_ptr<SlabAllocator>, Cpu::kMaxCpus> cpu_allocators_;
};

}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_SLABALLOCATOR_H_
