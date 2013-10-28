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
#ifndef EBBRT_EBB_CONSOLE_CONSOLE_HPP
#define EBBRT_EBB_CONSOLE_CONSOLE_HPP

#include <functional>

#include "lrt/config.hpp"
#include "ebb/ebb.hpp"
#include "misc/buffer.hpp"

namespace ebbrt {
  class Console : public EbbRep {
  public:
    virtual Buffer Alloc(size_t size) = 0;
    virtual void Write(Buffer buffer) = 0;
  protected:
    Console(EbbId id) : EbbRep{id} {}
  };

  const EbbRef<Console> console =
    EbbRef<Console>(lrt::config::find_static_ebb_id(nullptr,"Console"));

}
#endif
