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

#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ebb/ebb.hpp"
#include "ebb/EventManager/Future.hpp"
#include "ebb/HashTable/RemoteHashTable.hpp"
#include "misc/network.hpp"

namespace ebbrt {
  class DataflowCoordinator : public EbbRep {
  public:
    class Executor : public EbbRep {
    public:
      virtual Future<std::vector<Buffer> >
      Execute(const std::string& task, std::vector<Buffer> inputs) = 0;
    protected:
      Executor(EbbId id) : EbbRep{id} {}
    };
    struct TaskDescriptor {
      std::string task;
      std::vector<std::string> inputs;
      std::vector<std::string> outputs;
    };

    struct DataDescriptor {
      DataDescriptor() : exists{false} {}
      DataDescriptor(std::string prod) : producer{prod} {}
      std::string producer;
      std::unordered_set<std::string> consumers;
      bool exists;
      NetworkId location;
    };

    typedef std::unordered_map<std::string, TaskDescriptor> TaskTable;
    typedef std::unordered_map<std::string, DataDescriptor> DataTable;

    virtual Future<void> Execute(TaskTable task_table,
                                 DataTable data_table,
                                 EbbRef<Executor> executor,
                                 EbbRef<RemoteHashTable> hash_table)= 0;
  protected:
    DataflowCoordinator(EbbId id) : EbbRep{id} {}
  };
}

#endif
