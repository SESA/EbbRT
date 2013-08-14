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

constexpr ebbrt::app::Config::InitEbb late_init_ebbs[] = {
  { .name = "EbbManager" },
  { .name = "EventManager" },
  { .name = "MessageManager" },
  { .name = "DataflowCoordinator" },
  { .name = "MCPhotonExecutor" },
  { .name = "RemoteHashTable" }
};

constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "EbbManager", .id = 2},
  {.name = "EventManager", .id = 5},
  {.name = "MessageManager", .id = 7},
  {.name = "DataflowCoordinator", .id = 8},
  {.name = "MCPhotonExecutor", .id = 9},
  {.name = "RemoteHashTAble", .id = 10}
};

const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 0,
  .num_late_init = sizeof(late_init_ebbs) / sizeof(Config::InitEbb),
  .late_init_ebbs = late_init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};
