#ifndef EBBRT_DEVICE_VIRTIO_NET_HPP
#define EBBRT_DEVICE_VIRTIO_NET_HPP

#include "device/virtio.hpp"

namespace ebbrt {
  class VirtioNet : public Virtio {
  public:
    explicit VirtioNet(pci::Device& device);
    bool HasMacAddress();
    void GetMacAddress(uint8_t (*address)[6]);
  private:
    bool msi_x_enabled;
  };
}

#endif
