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
#include <string>
#include <functional>
#include <unordered_map>

#include "lrt/EbbId.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      class EbbRoot;
    }
  }
  namespace app {
    /* symbol table for configuration */
    // fixme, don't return root, for now needed to fill initial root table
    typedef lrt::trans::EbbRoot* (*ConfigFuncPtr)();
    void AddSymbol (std::string str, ConfigFuncPtr);
    /// lookup return function poitner
    ConfigFuncPtr LookupSymbol (std::string str);
    /**
     * Application configuration.
     * The configuration defines which
     * ebbrt::lrt::trans::EbbRoot%s should be constructed
     * and which ebbrt::lrt::trans::EbbId%s are statically known.
     */
    struct Config {
      /** The space this node should allocate
          ebbrt::lrt::trans::EbbId%s out of */
      uint16_t space_id;

      /**
       * An element used to statically construct Ebbs.
       */
      class InitEbb {
      public:
        /** The id to install the root on */
        const char* name;
      };

      /** Number of Ebbs to statically construct early
          These Ebbs cannot rely on globally constructed objects or
          exceptions at construction time */
#ifdef __ebbrt__
      size_t num_early_init;
      const InitEbb* early_init_ebbs; // ebbs pre-memory allocator
#endif
      size_t num_late_init;
      /** Array describing which Ebbs to statically construct */
      const InitEbb* late_init_ebbs;
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
