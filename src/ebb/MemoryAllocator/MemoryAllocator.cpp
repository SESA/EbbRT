#include "ebb/MemoryAllocator/MemoryAllocator.hpp"

#ifdef LRT_BARE
char ebbrt::mem_allocator_id_resv __attribute__ ((section ("static_ebb_ids")));
#endif
