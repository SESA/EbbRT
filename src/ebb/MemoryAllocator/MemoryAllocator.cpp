#include "ebb/MemoryAllocator/MemoryAllocator.hpp"

char mem_allocator_id_resv __attribute__ ((section ("static_ebb_ids")));
extern char static_ebb_ids_start[];
ebbrt::Ebb<ebbrt::MemoryAllocator> ebbrt::memory_allocator =
  ebbrt::Ebb<MemoryAllocator>(static_cast<ebbrt::EbbId>
                     (&mem_allocator_id_resv - static_ebb_ids_start));
