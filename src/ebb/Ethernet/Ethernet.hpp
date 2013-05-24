#ifndef EBBRT_EBB_ETHERNET_ETHERNET_HPP
#define EBBRT_EBB_ETHERNET_ETHERNET_HPP

#include <functional>

#include "ebb/ebb.hpp"
#include "misc/buffer.hpp"

namespace ebbrt {
  class Ethernet : public EbbRep {
  public:
    virtual void Send(char mac_addr[6],
                      uint16_t ethertype,
                      BufferList buffers,
                      const std::function<void(BufferList)>& cb = nullptr) = 0;
  };
  extern EbbRef<Ethernet> ethernet;
}

#endif
