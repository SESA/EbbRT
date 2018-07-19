//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef HOSTED_SRC_INCLUDE_EBBRT_POOLALLOCATOR_H_
#define HOSTED_SRC_INCLUDE_EBBRT_POOLALLOCATOR_H_

#include <iostream>
#include <numeric>
#include <sstream>
#include <string>

#include "../StaticSharedEbb.h"
#include "EbbRef.h"
#include "Messenger.h"
#include "NodeAllocator.h"
#include "StaticIds.h"

namespace ebbrt {
class PoolAllocator : public StaticSharedEbb<PoolAllocator> {
 private:
  ebbrt::Messenger::NetworkId* nids_;
  int num_nodes_;
  std::atomic<int> num_nodes_alloc_;
  std::string binary_path_;
  ebbrt::Promise<void> pool_promise_;
  std::vector<std::string> nodes_;

 public:
  std::string RunCmd(std::string cmd);
  void AllocatePool(std::string binary_path, int numNodes,
                    std::vector<size_t> cpu_ids);
  void AllocatePool(std::string binary_path, int numNodes) {
    std::vector<size_t> cpu_ids(numNodes);
    std::iota(std::begin(cpu_ids), std::end(cpu_ids), 0);
    AllocatePool(binary_path, numNodes, cpu_ids);
  }
  void AllocateNode(int i);
  ebbrt::NodeAllocator::NodeDescriptor GetNodeDescriptor(int i);
  ebbrt::Messenger::NetworkId GetNidAt(int i) { return nids_[i]; };
  ebbrt::Future<void> waitPool() {
    return std::move(pool_promise_.GetFuture());
  }
  void EmptyPool() {
    node_allocator->FreeAllNodes();
    node_allocator->RemoveNetwork();
  }
};
const constexpr auto pool_allocator = EbbRef<PoolAllocator>(kPoolAllocatorId);
}
#endif  // HOSTED_SRC_INCLUDE_EBBRT_POOLALLOCATOR_H_
