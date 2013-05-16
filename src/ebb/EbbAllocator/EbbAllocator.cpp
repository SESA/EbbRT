#include "ebb/EbbAllocator/EbbAllocator.hpp"

char ebb_allocator_id_resv __attribute__ ((section ("static_ebb_ids")));
extern char static_ebb_ids_start[];
ebbrt::Ebb<ebbrt::EbbAllocator> ebbrt::ebb_allocator
__attribute__((init_priority (101))) =
  ebbrt::Ebb<EbbAllocator>(static_cast<ebbrt::EbbId>
                              (&ebb_allocator_id_resv - static_ebb_ids_start));
