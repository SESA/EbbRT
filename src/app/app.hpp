#ifndef EBBRT_APP_APP_HPP
#define EBBRT_APP_APP_HPP

#include "lrt/trans.hpp"

namespace ebbrt {
  namespace app {
    class Config {
    public:
      uint16_t node_id;
      class InitEbb {
      public:
        lrt::trans::EbbId id;
        lrt::trans::EbbRoot* (*create_root)();
      };
      size_t num_init;
      const InitEbb* init_ebbs;
    };

    extern const Config config;
    void start();
  }
}

#endif
