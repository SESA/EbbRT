#ifndef EBBRT_LRT_BOOT_HPP
#error "Don't include this file directly"
#endif

#include <arch/x86_64/multiboot.hpp>

namespace ebbrt {
  namespace lrt {
    namespace boot {
      extern const MultibootInformation* multiboot_information;
    }
  }
}
