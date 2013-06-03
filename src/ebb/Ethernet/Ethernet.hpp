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
    virtual const uint8_t* MacAddress() = 0;
    virtual void SendComplete() = 0;
    virtual void Register(uint16_t ethertype,
                          std::function<void(const uint8_t*, size_t)> func) = 0;
    virtual void Receive() = 0;
  };
  extern EbbRef<Ethernet> ethernet;
}

#endif
