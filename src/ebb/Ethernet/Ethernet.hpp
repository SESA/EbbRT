#ifndef EBBRT_EBB_ETHERNET_ETHERNET_HPP
#define EBBRT_EBB_ETHERNET_ETHERNET_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class Ethernet : public EbbRep {
  public:
    virtual void* Allocate(size_t size) = 0;
  };
}

#endif
