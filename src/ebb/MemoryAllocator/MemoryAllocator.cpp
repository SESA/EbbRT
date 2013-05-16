#include "ebb/MemoryAllocator/MemoryAllocator.hpp"

char mem_allocator_id_resv __attribute__ ((section ("static_ebb_ids")));
extern char static_ebb_ids_start[];
const ebbrt::Ebb<ebbrt::MemoryAllocator> ebbrt::memory_allocator
__attribute__((init_priority (101))) =
  ebbrt::Ebb<MemoryAllocator>(static_cast<ebbrt::EbbId>
                     (&mem_allocator_id_resv - static_ebb_ids_start));
