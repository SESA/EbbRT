#include "arch/x86_64/multiboot.hpp"
#include "lrt/boot.hpp"
#include "lrt/mem_impl.hpp"


bool
ebbrt::lrt::mem::init(unsigned num_cores)
{
  /**@brief Partition available address space into core-specific regions */
  if (!boot::multiboot_information->has_mem) {
    return false;
  }
  /* regions array stored at start of free memory */
  regions = reinterpret_cast<Region*>(mem_start);
  char* ptr = mem_start + (num_cores * sizeof(Region));
  /* remaining memory is calculated and divided equally between cores */
  uint64_t num_bytes = static_cast<uint64_t>
    (boot::multiboot_information->memory_higher) << 10;
  /* 1MB being the start of the kernel */
  num_bytes -= reinterpret_cast<uint64_t>(mem_start) - 0x100000;
  for (unsigned i = 0; i < num_cores; ++i) {
    regions[i].start = regions[i].current = ptr;
    ptr += num_bytes / num_cores;
    regions[i].end = ptr;
  }
  return true;
}
