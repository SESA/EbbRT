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
     * The entry point to the application.
     * Upon return the system enters the event loop and begins
     * dispatching events. This function should establish event
     * handlers and return.
     */
    void start();
  }
}

#endif
