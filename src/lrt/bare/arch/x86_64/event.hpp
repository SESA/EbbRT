#ifndef EBBRT_LRT_EVENT_HPP
#error "Don't include this file directly"
#endif

#include "arch/x86_64/acpi.hpp"

namespace {
  int num_cores = -1;
}

namespace ebbrt {
  namespace lrt {
    namespace event {
      inline unsigned
      get_num_cores()
      {
        if (num_cores == -1) {
          acpi::init();
          num_cores = acpi::get_num_cores();
        }
        return num_cores;
      }

      inline Location
      get_location()
      {
        int loc;
        asm ("mov %%fs:(0), %[loc]"
             : [loc] "=r" (loc));
        return loc;
      }
    }
  }
}
