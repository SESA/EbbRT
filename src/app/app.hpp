#ifndef EBBRT_APP_APP_HPP
#define EBBRT_APP_APP_HPP

#include <src/lrt/trans.hpp>

namespace ebbrt {
  namespace app {
    /**
     * Application configuration.
     * The configuration defines which
     * ebbrt::lrt::trans::EbbRoot%s should be constructed
     * and which ebbrt::lrt::trans::EbbId%s are statically known.
     */
    class Config {
    public:
      /** The space this node should allocate
          ebbrt::lrt::trans::EbbId%s out of */
      uint16_t space_id;
      /** Size of ebbrt::app::Config::init_ebbs */
      size_t num_init;
      /**
       * An element used to statically construct Ebbs.
       */
      class InitEbb {
      public:
        /** The function to construct the root */
        lrt::trans::EbbRoot* (*create_root)();
        /** The id to install the root on */
        lrt::trans::EbbId id;
      };
      /** Array describing which Ebbs to statically construct */
      const InitEbb* init_ebbs;
      /** Size of ebbrt::app::Config::static_ebb_ids */
      size_t num_statics;
      /**
       * Defines a statically known EbbId.
       */
      class StaticEbbId {
      public:
        /** Name to lookup */
        const char* name;
        /** The associated Id */
        lrt::trans::EbbId id;
      };
      /** Array describing the statically known
          ebbrt::lrt::trans::EbbId%s */
      const StaticEbbId* static_ebb_ids;
    };
    /**
     * The configuration which is statically linked in to
     * the application.
     * This is defined by each application.
    */
    extern const Config config;

#ifdef __ebbrt__
    /**
     *  Whether or not to enter ebbrt::app::start() on all cores or
     *  just one.
     *  This is by default weakly defined in ./lrt/boot.cpp
     *  to false. An application can override it.
     *
     *  @hideinitializer
    */
    extern bool multi;
#endif

    /**
     * The entry point to the application.
     * Upon return the system enters the event loop and begins
     * dispatching events. This function should establish event
     * handlers and return.
     */
    void start();
  }
}

#endif
