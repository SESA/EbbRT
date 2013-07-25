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
#ifndef EBBRT_APP_HIDDENREGISTER__HIDDEN_HPP
#define EBBRT_APP_HIDDENREGISTER__HIDDEN_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class Hidden : public EbbRep {
  public:
    static EbbRoot* ConstructRoot();
    Hidden();

    virtual void NoReturn();

    struct SmallStruct {
      uint64_t v0;
      uint64_t v1;
    };

    virtual SmallStruct SmallPODReturn();

    struct BigStruct {
      uint64_t vals[16];
    };

    virtual BigStruct BigPODReturn();
  };
}
#endif
