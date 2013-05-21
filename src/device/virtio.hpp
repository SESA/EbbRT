#ifndef EBBRT_DEVICE_VIRTIO_HPP
#define EBBRT_DEVICE_VIRTIO_HPP

#include "misc/pci.hpp"

namespace ebbrt {
  class Virtio {
  public:
    class QueueDescriptor {
    public:
      uint64_t address;
      uint32_t length;
      union {
        uint16_t raw;
        struct {
          uint16_t next :1;
          uint16_t write :1;
          uint16_t indirect :1;
          uint16_t :13;
        };
      } flags;
      uint16_t next;
    };
    class Available {
    public:
      union {
        uint16_t raw;
        struct {
          uint16_t no_interrupt :1;
          uint16_t :15;
        };
      };
      uint16_t index;
      uint16_t ring[];
    };
    class UsedElement {
    public:
      uint32_t index;
      uint32_t length;
    };
    class Used {
    public:
      union {
        uint16_t raw;
        struct {
          uint16_t no_notify :1;
          uint16_t :15;
        };
      };
      uint16_t index;
      UsedElement ring[];
    };
    explicit Virtio(pci::Device& device);
    uint32_t DeviceFeatures();
    uint32_t GuestFeatures();
    void GuestFeatures(uint32_t features);
    uint32_t QueueAddress();
    void QueueAddress(uint32_t address);
    uint16_t QueueSize();
    uint16_t QueueSelect();
    void QueueSelect(uint16_t queue_select);
    uint16_t QueueNotify();
    void QueueNotify(uint16_t queue_notify);
    union device_status_t {
      uint8_t raw;
      struct {
        uint8_t acknowledge :1;
        uint8_t driver :1;
        uint8_t driver_ok :1;
        uint8_t :4;
        uint8_t failed :1;
      };
    };
    device_status_t DeviceStatus();
    void Acknowledge();
    void Driver();
    void DriverOK();
    void DeviceStatus(device_status_t device_status);
    uint8_t ISRStatus();

    // Only if MSI-X is enabled!
    uint16_t ConfigurationVector();
    void ConfigurationVector(uint16_t configuration_vector);
    uint16_t QueueVector();
    void QueueVector(uint16_t queue_vector);

    // Helper
    size_t QszBytes(uint16_t qsz);
  protected:
    const pci::Device& device_;
    uint16_t io_addr_;
  };
}
#endif
