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

#include "app/app.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebbs.hpp"

constexpr ebbrt::app::Config::InitEbb init_ebbs[] = {
  {
    .create_root = ebbrt::PrimitiveEbbManagerConstructRoot,
    .name = "EbbManager"
  },
  {
    .create_root = ebbrt::fox::RDData::ConstructRoot,
    .name = STR_RDDATA
  },
  {
    .create_root = ebbrt::fox::TaskQ::ConstructRoot,
    .name = STR_TASKS
  },
  {
    .create_root = ebbrt::fox::TaskSync::ConstructRoot,
    .name = STR_TASK_SYNC
  },
  {
    .create_root = ebbrt::fox::RWData::ConstructRoot,
    .name = STR_RWDATA
  },
  {
    .create_root = ebbrt::fox::Hash::ConstructRoot,
    .name = STR_HASH
  }
};

constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "EbbManager",  .id = 2},
  {.name = STR_RDDATA,    .id = 3},
  {.name = STR_TASKS,     .id = 4},
  {.name = STR_TASK_SYNC, .id = 5},
  {.name = STR_RWDATA,    .id = 6},
  {.name = STR_HASH,      .id = 7}
};

const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 0,
  .num_init = sizeof(init_ebbs) / sizeof(Config::InitEbb),
  .init_ebbs = init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};
