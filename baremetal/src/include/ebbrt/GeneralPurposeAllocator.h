//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_GENERALPURPOSEALLOCATOR_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_GENERALPURPOSEALLOCATOR_H_

#include <array>

#include <ebbrt/CacheAligned.h>
#include <ebbrt/Debug.h>
#include <ebbrt/SlabAllocator.h>
#include <ebbrt/Trans.h>

namespace ebbrt {
template <size_t... sizes_in>
class GeneralPurposeAllocator : public CacheAligned {
 public:
  static void Init() {
    rep_allocator =
        new SlabAllocatorRoot(sizeof(GeneralPurposeAllocator<sizes_in...>),
                              alignof(GeneralPurposeAllocator<sizes_in...>));

    Construct<0, sizes_in...> c;
    c(allocator_roots);
  }

  static GeneralPurposeAllocator<sizes_in...>& HandleFault(EbbId id) {
    if (reps[Cpu::GetMine()] == nullptr) {
      reps[Cpu::GetMine()] = new GeneralPurposeAllocator<sizes_in...>();
    }

    auto& allocator = *reps[Cpu::GetMine()];
    EbbRef<GeneralPurposeAllocator<sizes_in...>>::CacheRef(id, allocator);
    return allocator;
  }

  GeneralPurposeAllocator() {
    for (size_t i = 0; i < allocators_.size(); ++i) {
      allocators_[i] = &allocator_roots[i]->GetCpuAllocator();
    }
  }

  void* operator new(size_t size) {
    auto& allocator = rep_allocator->GetCpuAllocator();
    auto ret = allocator.Alloc();

    if (ret == nullptr)
      throw std::bad_alloc();

    return ret;
  }

  void operator delete(void* p) { UNIMPLEMENTED(); }

  void* Alloc(size_t size, Nid nid = Cpu::GetMyNode()) {
    Indexer<0, sizes_in...> i;
    auto index = i(size);
    kbugon(index == -1, "Attempt to allocate %zu bytes not supported\n", size);
    auto ret = allocators_[index]->Alloc(nid);
    kbugon(ret == nullptr,
           "Failed to allocate from this NUMA node, should try others\n");
    return ret;
  }

  void Free(void* p) {
    auto page = mem_map::AddrToPage(p);
    kassert(page != nullptr);

    auto& allocator = page->data.slab_data.cache->root_.GetCpuAllocator();
    allocator.Free(p);
  }

 private:
  template <size_t index, size_t... tail> struct Construct {
    void
    operator()(std::array<SlabAllocatorRoot*, sizeof...(sizes_in)>& roots) {}
  };

  template <size_t index, size_t head, size_t... tail>
  struct Construct<index, head, tail...> {
    static_assert(head <= SlabAllocator::kMaxSlabSize,
                  "gp allocator instantiation failed, "
                  "request for slab allocator is too "
                  "large");
    void
    operator()(std::array<SlabAllocatorRoot*, sizeof...(sizes_in)>& roots) {
      roots[index] = new SlabAllocatorRoot(head);
      Construct<index + 1, tail...> next;
      next(roots);
    }
  };

  template <size_t index, size_t... tail> struct Indexer {
    size_t operator()(size_t size) { return -1; }
  };

  template <size_t index, size_t head, size_t... tail>
  struct Indexer<index, head, tail...> {
    size_t operator()(size_t size) {
      if (size <= head) {
        return index;
      } else {
        Indexer<index + 1, tail...> i;
        return i(size);
      }
    }
  };

  static std::array<SlabAllocatorRoot*, sizeof...(sizes_in)> allocator_roots;
  static std::array<GeneralPurposeAllocator<sizes_in...>*, Cpu::kMaxCpus> reps;
  static SlabAllocatorRoot* rep_allocator;
  std::array<SlabAllocator*, sizeof...(sizes_in)> allocators_;
};

template <size_t... sizes_in>
std::array<SlabAllocatorRoot*, sizeof...(sizes_in)>
GeneralPurposeAllocator<sizes_in...>::allocator_roots;

template <size_t... sizes_in>
std::array<GeneralPurposeAllocator<sizes_in...>*, Cpu::kMaxCpus>
GeneralPurposeAllocator<sizes_in...>::reps;

template <size_t... sizes_in>
SlabAllocatorRoot* GeneralPurposeAllocator<sizes_in...>::rep_allocator;

// roughly equivalent to the breakdown Linux uses, may need tuning
typedef GeneralPurposeAllocator<
    8, 16, 32, 64, 96, 128, 192, 256, 512, 1024, 2 * 1024, 4 * 1024, 8 * 1024,
    16 * 1024, 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024, 512 * 1024,
    1024 * 1024, 2 * 1024 * 1024, 4 * 1024 * 1024,
    8 * 1024 * 1024> GeneralPurposeAllocatorType;

constexpr auto gp_allocator =
    EbbRef<GeneralPurposeAllocatorType>(kGpAllocatorId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_GENERALPURPOSEALLOCATOR_H_
