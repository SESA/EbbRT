#ifndef EBBRT_EBB_ETHERNET_ETHERNET_HPP
#define EBBRT_EBB_ETHERNET_ETHERNET_HPP

#include <functional>

#include "ebb/ebb.hpp"
#include "misc/buffer.hpp"

namespace ebbrt {
  class Ethernet : public EbbRep {
  public:
    class Header {
    public:
      uint8_t destination[6];
      uint8_t source[6];
      uint16_t ethertype;
    } __attribute__((packed));
    virtual void Send(BufferList buffers,
                      std::function<void()> cb = nullptr) = 0;
    virtual const char* MacAddress() = 0;
    virtual void SendComplete() = 0;
  };
  extern EbbRef<Ethernet> ethernet;
}

#endif
