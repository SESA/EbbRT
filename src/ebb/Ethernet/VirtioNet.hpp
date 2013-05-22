#ifndef EBBRT_EBB_ETHERNET_VIRTIONET_HPP
#define EBBRT_EBB_ETHERNET_VIRTIONET_HPP

#include "ebb/Ethernet/Ethernet.hpp"
#include "device/virtio.hpp"

namespace ebbrt {
  class VirtioNet : public Ethernet {
  public:
    static EbbRoot* ConstructRoot();
    VirtioNet();
    void* Allocate(size_t size) override;
  private:
    uint16_t io_addr_;
    uint16_t send_max_;
    virtio::QueueDescriptor* send_descs_;
    virtio::Available* send_available_;
    virtio::Used* send_used_;
  };
}

#endif
