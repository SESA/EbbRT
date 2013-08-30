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
#include <iostream>
#include <vector>

#include <mpi.h>

#include "app/app.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/DataflowCoordinator/SingleDataflowCoordinator.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

ebbrt::EbbRoot*
ebbrt::SingleDataflowCoordinator::ConstructRoot()
{
  return new SharedRoot<SingleDataflowCoordinator>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol("DataflowCoordinator",
                        ebbrt::SingleDataflowCoordinator::ConstructRoot);
}

ebbrt::SingleDataflowCoordinator::SingleDataflowCoordinator(EbbId id)
  : DataflowCoordinator{id}
{
}

ebbrt::Future<void>
ebbrt::SingleDataflowCoordinator::Execute(TaskTable task_table,
                                          DataTable data_table,
                                          EbbRef<Executor> executor,
                                          EbbRef<RemoteHashTable> hash_table,
                                          EbbRef<AccrualFailureDetector> fd)
{
  task_table_ = std::move(task_table);
  data_table_ = std::move(data_table);
  executor_ = executor;
  hash_table_ = hash_table;
  fd_ = fd;

  int size;
  if (MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS) {
    throw std::runtime_error("MPI_Comm_size failed");
  }

  std::function<void(NetworkId,bool)> failure =
    [=](NetworkId who, bool fail) {
    auto me = EbbRef<SingleDataflowCoordinator>{ebbid_};
    me->Failure(std::move(who), fail);
  };

  for (int i = 1; i < size; ++i) {
    NetworkId id;
    id.rank = i;
    idle_workers_.insert(id);
    fd_->Register(id, failure);
  }

  for (const auto& kv : task_table_) {
    if (TaskRunnable(kv.second)) {
      runnable_tasks_.insert(kv.first);
    }
    if (EndTask(kv.second)) {
      end_tasks_.insert(kv.first);
    }
  }

  while (!runnable_tasks_.empty() && !idle_workers_.empty()) {
    auto taskit = runnable_tasks_.begin();
    auto workerit = idle_workers_.begin();
    Schedule(*taskit, *workerit);
    runnable_tasks_.erase(taskit);
    idle_workers_.erase(workerit);
  }

  return promise_.GetFuture();
}

void
ebbrt::SingleDataflowCoordinator::Failure(NetworkId who, bool fail)
{
  if (fail) {
    std::cout << who.rank << " failed" << std::endl;
    running_workers_.erase(who);
    idle_workers_.erase(who);
    failed_workers_.insert(who);

    //iterate over all tasks that were executed/executing on the failed node
    auto itpair = task_locations_.equal_range(who);
    for (auto it = itpair.first; it != itpair.second; ++it) {
      const auto& taskstr = it->second;
      auto& task = task_table_[taskstr];

      task.running = false;
      // If we can find a path to an end task that has not been
      // executed and none of the data along the path exists, then we
      // must reproduce this data
      bool reexecute = false;
      // depth first search
      auto to_search = std::stack<std::string>{};
      to_search.push(taskstr);
      while (!to_search.empty() && !reexecute) {
        auto taskstr = std::move(to_search.top());
        to_search.pop();
        runnable_tasks_.erase(taskstr);
        auto& task = task_table_[taskstr];

        //If this task is an end task that has not been executed, then
        //we must reexecute this path
        if (EndTask(task) && task.must_be_executed && !task.running) {
          reexecute = true;
          break;
        }

        //Check all outputs
        for (const auto& datastr : task.outputs) {
          auto& data = data_table_[datastr];
          //Report any lost data as not existing
          if (data.exists && data.location == who) {
            data.exists = false;
          }

          //If the data doesn't exist, then lets check its consumers
          if (!data.exists) {
            for (const auto& taskstr : data.consumers) {
              auto& task = task_table_[taskstr];
              if (!task.running) {
                to_search.push(taskstr);
              }
            }
          }
        }
      }

      //If we have to reexecute the task, we must search up the tree
      //to find the set of tasks that must be reexecuted
      if (reexecute) {
        auto to_search = std::stack<std::string>{};
        to_search.push(taskstr);
        while (!to_search.empty()) {
          auto taskstr = std::move(to_search.top());
          to_search.pop();
          auto& task = task_table_[taskstr];

          if (!task.running) {
            task.must_be_executed = true;

            //Check if we can execute this task now
            bool can_reexec = true;
            for (const auto& datastr : task.inputs) {
              auto& data = data_table_[datastr];
              //if an input doesnt exist then we can't reexec this
              //task so we must recursively check its producer
              if (!data.exists || data.location == who) {
                to_search.push(data.producer);
                can_reexec = false;
              }
            }
            if (can_reexec) {
              runnable_tasks_.insert(taskstr);
            }
          }
        }
      }
    }
    task_locations_.erase(itpair.first, itpair.second);
  } else {
    std::cout << who.rank << " resurrected" << std::endl;
    failed_workers_.erase(who);
    idle_workers_.insert(who);
  }

  while (!runnable_tasks_.empty() && !idle_workers_.empty()) {
    auto taskit = runnable_tasks_.begin();
    auto workerit = idle_workers_.begin();
    Schedule(*taskit, *workerit);
    runnable_tasks_.erase(taskit);
    idle_workers_.erase(workerit);
  }
}

bool
ebbrt::SingleDataflowCoordinator::TaskRunnable(const TaskDescriptor& task)
{
  for (const auto& data_id : task.inputs) {
    const auto& data = data_table_[data_id];
    if (!data.exists) {
      return false;
    }
  }
  return true;
}

bool
ebbrt::SingleDataflowCoordinator::EndTask(const TaskDescriptor& task)
{
  return task.outputs.empty();
}

void
ebbrt::SingleDataflowCoordinator::Schedule(const std::string& taskstr,
                                           NetworkId worker)
{
  //std::cout << "Scheduling " << taskstr << " to " << worker.rank << std::endl;

  running_workers_.emplace(worker, taskstr);

  auto& task = task_table_[taskstr];

  // std::cout << "Inputs: ";
  // for (const auto& input : task.inputs) {
  //   std::cout << input << " ";
  // }
  // std::cout << std::endl;

  // std::cout << "Outputs: ";
  // for (const auto& output : task.outputs) {
  //   std::cout << output << " ";
  // }
  // std::cout << std::endl;

  task.running = true;
  task_locations_.emplace(worker, taskstr);

  SingleDataflowCoordinatorMessage message;
  message.set_type(SingleDataflowCoordinatorMessage::STARTWORK);
  auto start_work = message.mutable_start_work();
  start_work->set_executor(executor_);
  start_work->set_hash_table(hash_table_);
  start_work->set_task_id(taskstr);
  start_work->set_task_name(task.task);
  for (const auto& input : task.inputs) {
    auto in = start_work->add_inputs();
    in->set_location(data_table_[input].location.rank);
    in->set_name(input);
  }
  for (const auto& output : task.outputs) {
    start_work->add_outputs(output);
  }

  auto buf = message_manager->Alloc(message.ByteSize());
  message.SerializeToArray(buf.data(), buf.length());
  message_manager->Send(worker, ebbid_, std::move(buf));
}

void
ebbrt::SingleDataflowCoordinator::HandleMessage(NetworkId from,
                                                Buffer buffer)
{
  //Ignore messages from failed workers
  if (failed_workers_.count(from)) {
    return;
  }

  SingleDataflowCoordinatorMessage message;
  message.ParseFromArray(buffer.data(), buffer.length());

  switch (message.type()) {
  case SingleDataflowCoordinatorMessage::STARTWORK:
    HandleStartWork(from, message.start_work());
    break;
  case SingleDataflowCoordinatorMessage::COMPLETEWORK:
    HandleCompleteWork(from, message.complete_work());
    break;
  }
}

void
ebbrt::SingleDataflowCoordinator::
HandleStartWork(NetworkId from,
                const SingleDataflowCoordinatorStartWork& message)
{
  auto hash_table = EbbRef<RemoteHashTable>{message.hash_table()};

  //Start input gets
  auto input_futs = std::vector<Future<Buffer> >(message.inputs_size());
  for (int i = 0; i < message.inputs_size(); ++i) {
    NetworkId from;
    from.rank = message.inputs(i).location();
    input_futs[i] = hash_table->Get(from,
                                    message.inputs(i).name());
  }

  when_all(input_futs).Then([=](Future<std::vector<Buffer> > inputs) {
    //once we get all inputs, execute the task
    auto executor = EbbRef<Executor>{message.executor()};
    return executor->Execute(message.task_name(), std::move(inputs.Get()));
  }).Then([=](Future<std::vector<Buffer> > outputs) {
      bool success = true;
      try {
        auto outs = outputs.Get();
        assert(outs.size() == message.outputs_size());
        //once we have all the outputs, store them
        for (int i = 0; i < message.outputs_size(); ++i) {
          hash_table->Set(message.outputs(i), std::move(outs[i]));
        }
      } catch (...) {
        success = false;
      }

      //Then reply that we have completed the task
      SingleDataflowCoordinatorMessage reply;
      reply.set_type(SingleDataflowCoordinatorMessage::COMPLETEWORK);
      auto complete_work = reply.mutable_complete_work();
      complete_work->set_task_id(message.task_id());
      complete_work->set_success(success);

      auto buf = message_manager->Alloc(reply.ByteSize());
      reply.SerializeToArray(buf.data(), buf.length());
      message_manager->Send(from, ebbid_, std::move(buf));
  });
}

void
ebbrt::SingleDataflowCoordinator::
HandleCompleteWork(NetworkId from,
                   const SingleDataflowCoordinatorCompleteWork& message)
{
  running_workers_.erase(from);
  idle_workers_.insert(from);

  if (message.success()) {
    end_tasks_.erase(message.task_id());
    if (end_tasks_.empty()) {
      promise_.SetValue();
      return;
    }

    running_workers_.erase(from);
    idle_workers_.insert(from);

    const auto& taskstr = message.task_id();
    //std::cout << taskstr << " completed" << std::endl;
    auto& task = task_table_[taskstr];
    task.must_be_executed = false;
    task.running = false;
    for (const auto& outputstr : task.outputs) {
      auto& output = data_table_[outputstr];
      output.exists = true;
      output.location = from;

      //iterate over the consumers of each output to see if they can be run
      for (const auto& consumerstr : output.consumers) {
        const auto& consumer = task_table_[consumerstr];
        if (TaskRunnable(consumer)) {
          if (!idle_workers_.empty()) {
            // If we have idle workers, just execute this task
            auto workerit = idle_workers_.begin();
            Schedule(consumerstr, *workerit);
            idle_workers_.erase(workerit);
          } else {
            // Otherwise, put it on the stack of runnable tasks
            runnable_tasks_.insert(consumerstr);
          }
        }
      }
    }
  } else {
    const auto& taskstr = message.task_id();
    std::cout << taskstr << " failed" << std::endl;
    auto& task = task_table_[taskstr];

    task.running = false;
    //delete the task from this location
    auto itpair = task_locations_.equal_range(from);
    for (auto it = itpair.first; it != itpair.second; ++it) {
      if (it->second == taskstr) {
        task_locations_.erase(it);
        break;
      }
    }

    bool can_execute = true;
    for (const auto& inputstr : task.inputs) {
      auto& input = data_table_[inputstr];
      if (!input.exists) {
        can_execute = false;
        break;
      }
    }

    //If we believe we can run this task still, then just reschedule
    if (can_execute) {
      Schedule(taskstr, from);
    } else {
      //If we have to reexecute the task, we must search up the tree
      //to find the set of tasks that must be reexecuted
      auto to_search = std::stack<std::string>{};
      to_search.push(taskstr);
      while (!to_search.empty()) {
        auto taskstr = std::move(to_search.top());
        to_search.pop();
        auto& task = task_table_[taskstr];

        if (!task.running) {
          task.must_be_executed = true;

          //Check if we can execute this task now
          bool can_reexec = true;
          for (const auto& datastr : task.inputs) {
            auto& data = data_table_[datastr];
            //if an input doesnt exist then we can't reexec this
            //task so we must recursively check its producer
            if (!data.exists) {
              to_search.push(data.producer);
              can_reexec = false;
            }
          }
          if (can_reexec) {
            runnable_tasks_.insert(taskstr);
          }
        }
      }
    }
  }

  while (!runnable_tasks_.empty() && !idle_workers_.empty()) {
    auto taskit = runnable_tasks_.begin();
    auto workerit = idle_workers_.begin();
    Schedule(*taskit, *workerit);
    runnable_tasks_.erase(taskit);
    idle_workers_.erase(workerit);
  }
}
