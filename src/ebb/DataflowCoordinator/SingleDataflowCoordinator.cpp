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
                                          EbbRef<RemoteHashTable> hash_table)
{
  task_table_ = std::move(task_table);
  data_table_ = std::move(data_table);
  executor_ = executor;
  hash_table_ = hash_table;

  int size;
  if (MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS) {
    throw std::runtime_error("MPI_Comm_size failed");
  }

  for (int i = 0; i < size; ++i) {
    NetworkId id;
    id.rank = i;
    idle_workers_.push(id);
  }

  for (const auto& kv : task_table_) {
    if (TaskRunnable(kv.second)) {
      runnable_tasks_.push(kv.first);
    }
    if (EndTask(kv.second)) {
      end_tasks_.insert(kv.first);
    }
  }

  while (!runnable_tasks_.empty() && !idle_workers_.empty()) {
    Schedule(runnable_tasks_.top(), idle_workers_.top());
    runnable_tasks_.pop();
    idle_workers_.pop();
  }

  return promise_.GetFuture();
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

  const auto& task = task_table_[taskstr];

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
    auto outs = outputs.Get();
    assert(outs.size() == message.outputs_size());
    //once we have all the outputs, store them
    for (int i = 0; i < message.outputs_size(); ++i) {
      hash_table->Set(message.outputs(i), std::move(outs[i]));
    }

    //Then reply that we have completed the task
    SingleDataflowCoordinatorMessage reply;
    reply.set_type(SingleDataflowCoordinatorMessage::COMPLETEWORK);
    auto complete_work = reply.mutable_complete_work();
    complete_work->set_task_id(message.task_id());

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
  end_tasks_.erase(message.task_id());
  if (end_tasks_.empty()) {
    promise_.SetValue();
    return;
  }

  idle_workers_.push(from);
  for (const auto& outputstr : task_table_[message.task_id()].outputs) {
    auto& output = data_table_[outputstr];
    output.exists = true;
    output.location = from;

    //iterate over the consumers of each output to see if they can be run
    for (const auto& consumerstr : output.consumers) {
      const auto& consumer = task_table_[consumerstr];
      if (TaskRunnable(consumer)) {
        if (!idle_workers_.empty()) {
          // If we have idle workers, just execute this task
          Schedule(consumerstr, idle_workers_.top());
          idle_workers_.pop();
        } else {
          // Otherwise, put it on the stack of runnable tasks
          runnable_tasks_.push(consumerstr);
        }
      }
    }
  }

  while (!runnable_tasks_.empty() && !idle_workers_.empty()) {
    Schedule(runnable_tasks_.top(), idle_workers_.top());
    runnable_tasks_.pop();
    idle_workers_.pop();
  }
}
