#ifndef EBBRT_APP_APP_HPP
#define EBBRT_APP_APP_HPP

#include <src/lrt/trans.hpp>

namespace ebbrt {
  namespace app {
    /**
     * @brief
     */
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
      size_t num_statics;
      class StaticEbbId {
      public:
        const char* name;
        lrt::trans::EbbId id;
      };
      StaticEbbId* static_ebb_ids;
    };

    extern const Config config;
    /**
     * @brief Entrance into ebb-world
     */
    void start();
  }
}

#endif
