#ifndef EBBRT_EBB_NODEALLOCATOR_NODEALLOCATOR_HPP
#define EBBRT_EBB_NODEALLOCATOR_NODEALLOCATOR_HPP

#include <string>
#include "ebb/ebb.hpp"
#include "misc/network.hpp"

namespace ebbrt {
  class NodeAllocator : public EbbRep {
  public:
    virtual NodeId Allocate(std::string path) = 0;
    virtual void Deallocate(NodeId target) = 0;
  };
  extern EbbRef<NodeAllocator> node_allocator;
}

#endif
