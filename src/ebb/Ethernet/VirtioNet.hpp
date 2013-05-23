#ifndef EBBRT_EBB_ETHERNET_VIRTIONET_HPP
#define EBBRT_EBB_ETHERNET_VIRTIONET_HPP

#include "ebb/Ethernet/Ethernet.hpp"
#include "device/virtio.hpp"
#include "sync/spinlock.hpp"

namespace ebbrt {
  class VirtioNet : public Ethernet {
  public:
    static EbbRoot* ConstructRoot();
    VirtioNet();
    void* Allocate(size_t size) override;
    void Send(void* buffer, size_t size) override;
  private:
    uint16_t io_addr_;
    uint16_t next_free_;
    uint16_t next_available_;
    Spinlock lock_;
    uint16_t send_max_;
    bool msix_enabled_;
    char mac_address_[6];
    virtio::QueueDescriptor* send_descs_;
    virtio::Available* send_available_;
    virtio::Used* send_used_;
  };
}

#endif
