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
#ifndef EBBRT_EBB_EBB_HPP
#define EBBRT_EBB_EBB_HPP

#include "lrt/event.hpp"
#include "lrt/EbbId.hpp"
#include "lrt/EbbRef.hpp"
#include "lrt/EbbRep.hpp"
#include "lrt/EbbRoot.hpp"
#include "lrt/Location.hpp"

namespace ebbrt {
  using lrt::trans::EbbRef;
  using lrt::trans::EbbId;
  using lrt::trans::EbbRep;
  using lrt::trans::EbbRoot;
  using lrt::event::Location;
  using lrt::event::get_location;
}

#endif
