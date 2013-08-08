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

#include <cstdlib>

//#include <mpi.h>

#include "ebbrt.hpp"
#include "ebb/DataflowCoordinator/DataflowCoordinator.hpp"

namespace {
  constexpr int DEFAULT_NPROC = 0;
  constexpr int DEFAULT_NPHOTONS = 5000;
  constexpr int DEFAULT_NTASKS = 1;
}

int main(int argc, char* argv[])
{
  int nprocs __attribute__((unused)) = DEFAULT_NPROC;
  int nphotons __attribute__((unused)) = DEFAULT_NPHOTONS;
  int ntasks __attribute__((unused)) = DEFAULT_NTASKS;
  bool nok = false;
  while (--argc > 0 && (*++argv)[0] == '-') {
    for (char* s = argv[0]+1; *s; s++) {
      switch (*s) {
      case 'n':
        if (isdigit(s[1])) nprocs = std::atoi(s+1);
        else nok = true;
        s += strlen(s+1);
        break;
      case 'p':
        if (isdigit(s[1])) nphotons = std::atoi(s+1);
        else nok = true;
        s += strlen(s+1);
        break;
      case 't':
        if (isdigit(s[1])) ntasks = std::atoi(s+1);
        else nok = true;
        s += strlen(s+1);
        break;
      default:
        nok = true;
        fprintf(stderr, " -- not an option: %c\n", *s);
        break;
      }
    }
  }

  if (nok || /*argc < 1 ||*/ (argc > 0 && *argv[0] == '?')) {
    fprintf(stderr, "Usage: %s -n<int> -p<int> -t<int>\n", argv[0]);
    fprintf(stderr, "  -n  number of processes, default: %d\n", DEFAULT_NPROC);
    fprintf(stderr, "  -p  photons per surface, default: %d\n", DEFAULT_NPHOTONS);
    fprintf(stderr, "  -t  tasks, default: %d\n", DEFAULT_NTASKS);
    exit(EXIT_FAILURE);
  }

  // int provided;
  // if (MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided) !=
  //     MPI_SUCCESS) {
  //   fprintf(stderr, "MPI_Init_thread failed");
  //   exit(-1);
  // }
  // if (provided != MPI_THREAD_MULTIPLE) {
  //   fprintf(stderr, "MPI does not provide proper thread support");
  //   exit(-1);
  // }

  int rank = 0;
  //MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  ebbrt::EbbRT instance;
  ebbrt::Context context{instance};

  if (rank == 0) {
    //TODO: this should be replicated for fault tolerance
    ebbrt::DataflowCoordinator::TaskTable task_table;
    ebbrt::DataflowCoordinator::DataTable data_table;

    ebbrt::DataflowCoordinator::DataTable::iterator data_it;
    ebbrt::DataflowCoordinator::TaskTable::iterator task_it;

    std::tie(data_it, std::ignore) =
      data_table.emplace("data0", ebbrt::DataflowCoordinator::DataDescriptor());
    data_it->second.consumers.insert("photon0");
    std::tie(data_it, std::ignore) =
      data_table.emplace("data1", ebbrt::DataflowCoordinator::DataDescriptor());
    data_it->second.consumers.insert("photon1");
    std::tie(data_it, std::ignore) =
      data_table.emplace("data2", ebbrt::DataflowCoordinator::DataDescriptor());
    data_it->second.consumers.insert("photon2");
    std::tie(data_it, std::ignore) =
      data_table.emplace("data3", ebbrt::DataflowCoordinator::DataDescriptor());
    data_it->second.consumers.insert("photon3");

    std::tie(task_it, std::ignore) =
      task_table.emplace("photon0",
                         ebbrt::DataflowCoordinator::TaskDescriptor());
    task_it->second.inputs.insert("data0");
    task_it->second.outputs.insert("photon_out0");
    std::tie(task_it, std::ignore) =
      task_table.emplace("photon1",
                         ebbrt::DataflowCoordinator::TaskDescriptor());
    task_it->second.inputs.insert("data1");
    task_it->second.outputs.insert("photon_out1");
    std::tie(task_it, std::ignore) =
      task_table.emplace("photon2",
                         ebbrt::DataflowCoordinator::TaskDescriptor());
    task_it->second.inputs.insert("data2");
    task_it->second.outputs.insert("photon_out2");
    std::tie(task_it, std::ignore) =
      task_table.emplace("photon3",
                         ebbrt::DataflowCoordinator::TaskDescriptor());
    task_it->second.inputs.insert("data3");
    task_it->second.outputs.insert("photon_out3");

    std::tie(data_it, std::ignore) =
      data_table.emplace("photon_out0",
                         ebbrt::DataflowCoordinator::DataDescriptor("photon0"));
    data_it->second.consumers.insert("reduce0_0");
    std::tie(data_it, std::ignore) =
      data_table.emplace("photon_out1",
                         ebbrt::DataflowCoordinator::DataDescriptor("photon1"));
    data_it->second.consumers.insert("reduce0_0");
    std::tie(data_it, std::ignore) =
      data_table.emplace("photon_out2",
                         ebbrt::DataflowCoordinator::DataDescriptor("photon2"));
    data_it->second.consumers.insert("reduce0_1");
    std::tie(data_it, std::ignore) =
      data_table.emplace("photon_out3",
                         ebbrt::DataflowCoordinator::DataDescriptor("photon3"));
    data_it->second.consumers.insert("reduce0_1");

    std::tie(task_it, std::ignore) =
      task_table.emplace("reduce0_0",
                         ebbrt::DataflowCoordinator::TaskDescriptor());
    task_it->second.inputs.insert("photon_out0");
    task_it->second.inputs.insert("photon_out1");
    task_it->second.outputs.insert("reduce_out0_0");
    std::tie(task_it, std::ignore) =
      task_table.emplace("reduce0_1",
                         ebbrt::DataflowCoordinator::TaskDescriptor());
    task_it->second.inputs.insert("photon_out2");
    task_it->second.inputs.insert("photon_out3");
    task_it->second.outputs.insert("reduce_out0_1");

    std::tie(data_it, std::ignore) =
      data_table.emplace("reduce_out0_0",
                         ebbrt::DataflowCoordinator::DataDescriptor("reduce0_0"));
    data_it->second.consumers.insert("reduce1_0");
    std::tie(data_it, std::ignore) =
      data_table.emplace("reduce_out0_1",
                         ebbrt::DataflowCoordinator::DataDescriptor("reduce0_1"));
    data_it->second.consumers.insert("reduce1_0");

    std::tie(task_it, std::ignore) =
      task_table.emplace("reduce1_0",
                         ebbrt::DataflowCoordinator::TaskDescriptor());
    task_it->second.inputs.insert("reduce_out0_0");
    task_it->second.inputs.insert("reduce_out0_1");
    task_it->second.outputs.insert("reduce_out1_0");

    std::tie(data_it, std::ignore) =
      data_table.emplace("reduce_out1_0",
                         ebbrt::DataflowCoordinator::DataDescriptor("reduce1_0"));

    ebbrt::EbbRef<ebbrt::DataflowCoordinator>
      dfcoord{ebbrt::lrt::trans::find_static_ebb_id("DataflowCoordinator")};

    dfcoord->Execute(std::move(task_table), std::move(data_table));
    // std::unordered_set<std::string> runnable_tasks;
    // std::unordered_set<int> idle_workers;

    // idle_workers.insert(0);
    // for (const auto& kv : task_table) {
    //   if (task_runnable(kv.second)) {
    //     runnable_tasks.insert(kv.first);
    //   }
    // }

    // for (auto& task : runnable_tasks) {
    //   std::cout << task << std::endl;
    // }

    // for (auto& worker : idle_workers) {
    //   std::cout << worker << std::endl;
    // }
  }
}

  // bool
  //   task_runnable(const TaskDescriptor& task) {
  //   for (const auto& data_id : task.inputs) {
  //     const auto& data = data_table[data_id];
  //     if (!data.producer.empty() && data.locations.empty()) {
  //       return false;
  //     }
  //   }
  //   return true;
  // }
