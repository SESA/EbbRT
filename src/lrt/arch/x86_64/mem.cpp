#include "arch/x86_64/multiboot.hpp"
#include "lrt/boot.hpp"
#include "lrt/mem_impl.hpp"

bool
ebbrt::lrt::mem::init(unsigned num_cores)
{
  if (!boot::multiboot_information->has_mem) {
    return false;
  }
  regions = reinterpret_cast<Region*>(mem_start);
  char* ptr = mem_start + (num_cores * sizeof(Region));
  uint64_t num_bytes = static_cast<uint64_t>
    (boot::multiboot_information->memory_higher) << 10;
  num_bytes -= reinterpret_cast<uint64_t>(mem_start) - 0x100000;
  for (unsigned i = 0; i < num_cores; ++i) {
    regions[i].start = regions[i].current = ptr;
    ptr += num_bytes / num_cores;
    regions[i].end = ptr;
  }
  return true;
}
