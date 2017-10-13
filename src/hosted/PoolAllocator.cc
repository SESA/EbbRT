//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef NDEBUG
#include <sys/socket.h>
#endif
#include <iostream>

#include "Debug.h"
#include "Cpu.h"
#include "Messenger.h"
#include "PoolAllocator.h"
#include "NodeAllocator.h"

#include "../GlobalIdMap.h"

const constexpr size_t kLineSize = 80;

std::string ebbrt::PoolAllocator::RunCmd(std::string cmd) {
  std::string out;
  char line[kLineSize];
  if (cmd.empty()) {
    return std::string();
  }
  auto f = popen(cmd.c_str(), "r");
  if (f == nullptr) {
    throw std::runtime_error("Failed to execute command: " + cmd);
  }
  while (std::fgets(line, kLineSize, f)) {
    out += line;
  }
  if (pclose(f) != 0) {
    throw std::runtime_error("Error reported by command: " + cmd + out);
  }
  if (!out.empty())
    out.erase(out.length() - 1);  // trim newline
  return out;
}

void ebbrt::PoolAllocator::AllocateNode(int i) {

  auto specified_nodes = nodes_.size() > 0;
  ebbrt::NodeAllocator::NodeArgs args = {};

  std::string node = (specified_nodes) ? nodes_[i % nodes_.size()] 
    : std::string();

  args.constraint_node = node;
  auto nd = ebbrt::node_allocator->AllocateNode(binary_path_, args);

  nd.NetworkId().Then(
      [this, i, specified_nodes, node](ebbrt::Future<ebbrt::Messenger::NetworkId> inner) {
    auto nid =inner.Get();
#ifndef NDEBUG
    std::cerr << "# allocated Node: " << i;
    if (specified_nodes)
      std:: cerr << " in: " << node;
    std::cerr << std::endl;
#endif
    num_nodes_alloc_.fetch_add(1);
    nids_[i] = nid;
    
    if (num_nodes_alloc_ == num_nodes_) {
      // Node ready to work, set future to true
      pool_promise_.SetValue(); 
    }
  });
}

void ebbrt::PoolAllocator::AllocatePool(std::string binary_path, 
    int num_nodes) {

  num_nodes_ = num_nodes;
  num_nodes_alloc_.store(0);
  binary_path_ = binary_path;
  nids_ = new ebbrt::Messenger::NetworkId [num_nodes_];

  char * str = getenv("EBBRT_NODE_LIST_CMD");
  std::string cmd = (str) ? std::string(str) : std::string();
  std::string node_list;
  if (!cmd.empty()) {
    node_list = RunCmd(cmd);
    char* node  = strtok (&node_list[0],",");
    while (node != NULL)
    {
      nodes_.push_back(node);
      node = strtok( NULL, "," );
    }
  }

  int cpu_num = ebbrt::Cpu::GetPhysCpus();

  std::cerr << "Pool Allocation Details: " << std::endl;
  std::cerr << "|      img: " << binary_path_ << std::endl;
  std::cerr << "|    nodes: " << num_nodes << std::endl;
  if (nodes_.size() > 0) {
    std::cerr << "| host ids: "; 
    for (size_t i = 0; i < nodes_.size(); i++)
      std::cerr << nodes_[i] << " ";
    std::cerr << std::endl;
  }
  std::cerr << "|     cpus: " << cpu_num << std::endl;

  // Round robin through the available cpus
  for (int i = 0; i < num_nodes; i++) {
    auto cpu_i = ebbrt::Cpu::GetByIndex(i % cpu_num);
    auto ctxt = cpu_i->get_context();

    ebbrt::event_manager->Spawn([this, i]() { AllocateNode(i); }, ctxt, true);
  }
}
