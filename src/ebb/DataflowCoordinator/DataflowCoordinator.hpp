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
#ifndef EBBRT_EBB_DATAFLOWCOORDINATOR_DATAFLOWCOORDINATOR_HPP
#define EBBRT_EBB_DATAFLOWCOORDINATOR_DATAFLOWCOORDINATOR_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ebb/ebb.hpp"
#include "ebb/EventManager/Future.hpp"
#include "misc/network.hpp"

namespace ebbrt {
  class DataflowCoordinator : public EbbRep {
  public:
    struct TaskDescriptor {
      std::unordered_set<std::string> inputs;
      std::unordered_set<std::string> outputs;
    };

    struct DataDescriptor {
      DataDescriptor() = default;
      DataDescriptor(std::string prod) : producer{prod} {}
      std::string producer;
      std::unordered_set<std::string> consumers;
      std::unordered_set<NetworkId> locations;
    };

    typedef std::unordered_map<std::string, TaskDescriptor> TaskTable;
    typedef std::unordered_map<std::string, DataDescriptor> DataTable;

    virtual Future<void> Execute(TaskTable task_table,
                                 DataTable data_table)= 0;
  protected:
    DataflowCoordinator(EbbId id) : EbbRep{id} {}
  };
}

#endif
