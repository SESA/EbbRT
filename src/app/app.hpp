/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef EBBRT_APP_APP_HPP
#define EBBRT_APP_APP_HPP

#include <cstring>

#include "lrt/EbbId.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      class EbbRoot;
    }
  }
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

      /**
       * An element used to statically construct Ebbs.
       */
      class InitEbb {
      public:
        /** The function to construct the root */
        lrt::trans::EbbRoot* (*create_root)();
        /** The id to install the root on */
        const char* name;
      };

#ifdef __ebbrt__
      /** Number of Ebbs to statically construct early
          These Ebbs cannot rely on globally constructed objects or
          exceptions at construction time */
      size_t num_early_init;
#endif
      /** Size of ebbrt::app::Config::init_ebbs */
      size_t num_init;
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
