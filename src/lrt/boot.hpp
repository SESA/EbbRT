#ifndef EBBRT_LRT_BOOT_HPP
#define EBBRT_LRT_BOOT_HPP

#include "lrt/arch/boot.hpp"
#include "lrt/trans.hpp"

namespace ebbrt {
  namespace lrt {
    namespace boot {
      class Config {
      public:
        uint16_t node;
        uint32_t count;
        class CreateRoot {
        public:
          trans::EbbId id;
          char symbol[0];
        };
        CreateRoot table[0];
      };

      void init() __attribute__((noreturn));
      void init_smp(unsigned num_cores) __attribute__((noreturn));
      void init_cpu();
      const Config* get_config();
      void* find_symbol(const char* name);
    }
  }
}

#endif
