#ifndef EBBRT_DEVICE_VIRTIO_HPP
#define EBBRT_DEVICE_VIRTIO_HPP

#include <ostream>

namespace ebbrt {
  class VirtioHeader {
  public:
    union {
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
    } hw_features; //RO
    union {
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
    } os_features; //RW
    uint32_t queue_address; //RW
    uint16_t queue_size; //RO
    uint16_t queue_select; //RW
    uint16_t queue_notify; //RW
    union DeviceStatus {
      uint8_t raw;
      struct {
        uint8_t acknowledge :1;
        uint8_t driver :1;
        uint8_t driver_ok :1;
        uint8_t :4;
        uint8_t failed :1;
      };
    } device_status;
    uint8_t isr_status; //RO
    // Only if MSI is enabled
    uint16_t msi_config_vector; //RW
    uint16_t msi_queue_vector; //RW
  };
  std::ostream& operator<<(std::ostream& os, const VirtioHeader& header);
}

#endif
