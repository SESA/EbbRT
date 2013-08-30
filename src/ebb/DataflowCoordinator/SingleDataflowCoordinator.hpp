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
#ifndef EBBRT_EBB_DATAFLOWCOORDINATOR_SINGLEDATAFLOWCOORDINATOR_HPP
#define EBBRT_EBB_DATAFLOWCOORDINATOR_SINGLEDATAFLOWCOORDINATOR_HPP

#include <unordered_map>
#include <unordered_set>

#include "ebb/DataflowCoordinator/DataflowCoordinator.hpp"
#include "ebb/DataflowCoordinator/SingleDataflowCoordinator.pb.h"

namespace ebbrt {
  class SingleDataflowCoordinator : public DataflowCoordinator {
  public:
    static EbbRoot* ConstructRoot();

    SingleDataflowCoordinator(EbbId id);

    virtual Future<void>
    Execute(TaskTable task_table, DataTable data_table,
            EbbRef<Executor> executor,
            EbbRef<RemoteHashTable> hash_table,
            EbbRef<AccrualFailureDetector> fd) override;

    virtual void
    HandleMessage(NetworkId from, Buffer buf) override;

  private:
    void CheckReexecute(const TaskDescriptor& task);
    void Failure(NetworkId who, bool fail);
    bool TaskRunnable(const TaskDescriptor& task);
    bool EndTask(const TaskDescriptor& task);
    void Schedule(const std::string& task, NetworkId worker);

    void HandleStartWork(NetworkId from,
                         const SingleDataflowCoordinatorStartWork& message);
    void HandleCompleteWork(NetworkId from,
                            const SingleDataflowCoordinatorCompleteWork& message);

    TaskTable task_table_;
    DataTable data_table_;

    std::unordered_set<NetworkId> idle_workers_;
    std::unordered_map<NetworkId, std::string> running_workers_;
    std::unordered_set<NetworkId> failed_workers_;

    std::unordered_set<std::string> runnable_tasks_;
    std::unordered_multimap<NetworkId, std::string> task_locations_;
    std::unordered_set<std::string> end_tasks_;

    Promise<void> promise_;

    EbbRef<Executor> executor_;
    EbbRef<RemoteHashTable> hash_table_;
    EbbRef<AccrualFailureDetector> fd_;
  };
};

#endif
