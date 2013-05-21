#include "arch/io.hpp"
#include "device/virtio_net.hpp"

namespace {
  union Features {
    uint32_t raw;
    struct {
      uint32_t csum :1;
      uint32_t guest_csum :1;
      uint32_t :3;
      uint32_t mac :1;
      uint32_t gso :1;
      uint32_t guest_tso4 :1;
      uint32_t guest_tso6 :1;
      uint32_t guest_ecn :1;
      uint32_t guest_ufo :1;
      uint32_t host_tso4 :1;
      uint32_t host_tso6 :1;
      uint32_t host_ecn :1;
      uint32_t host_ufo :1;
      uint32_t mrg_rxbuf :1;
      uint32_t status :1;
      uint32_t ctrl_vq :1;
      uint32_t ctrl_rx :1;
      uint32_t ctrl_vlan :1;
      uint32_t guest_announce :1;
    };
  };
}

ebbrt::VirtioNet::VirtioNet(pci::Device& device) :
  Virtio{device}, msi_x_enabled{false}
{
  Features features;
  features.raw = DeviceFeatures();

  Features supported_features;
  supported_features.raw = 0;
  supported_features.mac = features.mac;
  GuestFeatures(supported_features.raw);
}

bool
ebbrt::VirtioNet::HasMacAddress()
{
  Features features;
  features.raw = DeviceFeatures();
  return features.mac;
}

void
ebbrt::VirtioNet::GetMacAddress(uint8_t (*address)[6])
{
  uint16_t addr = io_addr_ + 20;
  if (msi_x_enabled) {
    addr += 4;
  }
  for (unsigned i = 0; i < 6; ++i) {
    (*address)[i] = in8(addr + i);
  }
}
