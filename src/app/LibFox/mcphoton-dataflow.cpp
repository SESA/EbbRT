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

#include <chrono>
#include <iostream>

#include <mpi.h>
#include <signal.h>
#include <unistd.h>

#include "ebbrt.hpp"
#include "app/LibFox/MCPhotonExecutor.hpp"
#include "ebb/DataflowCoordinator/DataflowCoordinator.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "ebb/FailureDetector/AccrualFailureDetector.hpp"

namespace {
  constexpr int DEFAULT_NPROC = 0;
  constexpr int DEFAULT_NPHOTONS = 5000;
  constexpr int DEFAULT_NTASKS = 1;
}

int rank;

void timeout(int signal)
{
  std::cout << rank << " failing" << std::endl;
  while (1)
    ;
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

  int provided;
  if (MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided) !=
      MPI_SUCCESS) {
    fprintf(stderr, "MPI_Init_thread failed");
    exit(-1);
  }
  if (provided != MPI_THREAD_MULTIPLE) {
    fprintf(stderr, "MPI does not provide proper thread support");
    exit(-1);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  ebbrt::EbbRT instance;
  ebbrt::Context context{instance};
  context.Activate();

  ebbrt::message_manager->StartListening();

  if (MPI_Barrier(MPI_COMM_WORLD) != MPI_SUCCESS) {
    fprintf(stderr, "MPI_Barrier failed");
    exit(-1);
  }

  if (rank == 0) {
    //TODO: this should be replicated for fault tolerance
    ebbrt::DataflowCoordinator::TaskTable task_table;
    ebbrt::DataflowCoordinator::DataTable data_table;

    ebbrt::DataflowCoordinator::DataTable::iterator data_it;
    ebbrt::DataflowCoordinator::TaskTable::iterator task_it;

    //Calculate the log of the number of tasks
    int logntasks = 0;
    {
      unsigned ntaskstmp = ntasks;
      while (ntaskstmp >>= 1) {
        ++logntasks;
      }
    }

    //The task dataflow graph is a tree, the first layer runs the
    //photon task and the other layers do a logarithmic reduction of
    //the outputs
    for (int i = 0; i <= logntasks; ++i) {
      for (int j = 0; j < (1 << (logntasks - i)); ++j) {
        auto append = std::to_string(i);
        append += "_";
        append += std::to_string(j);
        auto task = std::string{"task"};
        task += append;
        auto out = std::string{"out"};
        out += append;

        if (i == 0) {
          auto phot = std::string{"photon"};
          phot += std::to_string(j);
          task_table.insert({task, {phot, {}, {out}}});
        } else {
          auto inbase = std::string{"out"};
          inbase += std::to_string(i-1);
          inbase += "_";
          auto in0 = inbase;
          in0 += std::to_string(j*2);
          auto in1 = inbase;
          in1 += std::to_string(j*2 + 1);
          task_table.insert({task, {"reduce", {in0, in1}, {out}}});
        }

        auto consumer = std::string{"task"};
        consumer += std::to_string(i+1);
        consumer += "_";
        consumer += std::to_string(j / 2);
        data_table.insert({out, {task, {consumer}}});
      }
    }

    //The final task is to print the result of the last reduce
    auto task = std::string{"task"};
    task += std::to_string(logntasks + 1);
    task += "_0";
    auto in = std::string{"out"};
    in += std::to_string(logntasks);
    in += "_0";
    task_table.insert({task, {"print", {in}, {}}});

    ebbrt::EbbRef<ebbrt::DataflowCoordinator>
      dfcoord{ebbrt::lrt::trans::find_static_ebb_id("DataflowCoordinator")};

    ebbrt::EbbRef<ebbrt::MCPhotonExecutor>
      executor{ebbrt::lrt::trans::find_static_ebb_id("MCPhotonExecutor")};

    ebbrt::EbbRef<ebbrt::RemoteHashTable>
      hash_table{ebbrt::lrt::trans::find_static_ebb_id("RemoteHashTable")};

    auto fd = ebbrt::EbbRef<ebbrt::AccrualFailureDetector>
      {ebbrt::lrt::trans::find_static_ebb_id("PhiAccrualFailureDetector")};

    auto t1 = std::chrono::steady_clock::now();

    auto f = dfcoord->Execute(std::move(task_table), std::move(data_table),
                              executor, hash_table, fd);

    f.Then([=](ebbrt::Future<void> fut) {
        try {
          fut.Get();
        } catch (std::exception& e) {
          std::cerr << "Failed: " << e.what() << std::endl;
          return;
        } catch (...) {
          std::cerr << "Failed" << std::endl;
          return;
        }
        auto t2 = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast
          <std::chrono::duration<double>>(t2-t1).count();
        std::cout << "Wall time (sec) = " << duration << std::endl;
      });
  }

  signal(SIGALRM, timeout);
  // if (rank == 3) {
  //   alarm(30);
  // }

  context.Loop(-1);
}
