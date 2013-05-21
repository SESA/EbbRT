#include "arch/io.hpp"
#include "device/virtio.hpp"

ebbrt::Virtio::Virtio(pci::Device& device) :
  device_{device}, io_addr_{static_cast<uint16_t>(device.BAR0() & ~0x3)}
{
  device.BusMaster(true);
}

uint32_t
ebbrt::Virtio::DeviceFeatures()
{
  return in32(io_addr_);
}

uint32_t
ebbrt::Virtio::GuestFeatures()
{
  return in32(io_addr_ + 4);
}

void
ebbrt::Virtio::GuestFeatures(uint32_t features)
{
  out32(features, io_addr_ + 4);
}

void
ebbrt::Virtio::QueueAddress(uint32_t address)
{
  out32(address, io_addr_ + 8);
}

uint16_t
ebbrt::Virtio::QueueSize()
{
  return in16(io_addr_ + 12);
}

void
ebbrt::Virtio::QueueSelect(uint16_t queue_select)
{
  out16(queue_select, io_addr_ + 14);
}

void
ebbrt::Virtio::QueueNotify(uint16_t queue_notify)
{
  out16(queue_notify, io_addr_ + 16);
}

ebbrt::Virtio::device_status_t
ebbrt::Virtio::DeviceStatus()
{
  device_status_t status;
  status.raw = in8(io_addr_ + 18);
  return status;
}

void
ebbrt::Virtio::DeviceStatus(device_status_t status)
{
  out8(status.raw, io_addr_ + 18);
}

void
ebbrt::Virtio::Acknowledge()
{
  device_status_t status = DeviceStatus();
  status.acknowledge = true;
  DeviceStatus(status);
}

void
ebbrt::Virtio::Driver()
{
  device_status_t status = DeviceStatus();
  status.driver = true;
  DeviceStatus(status);
}

void
ebbrt::Virtio::DriverOK()
{
  device_status_t status = DeviceStatus();
  status.driver_ok = true;
  DeviceStatus(status);
}

size_t
ebbrt::Virtio::QszBytes(uint16_t qsz)
{
  return ((sizeof(Virtio::QueueDescriptor) * qsz +
           6 + 2 * qsz + 4095) & ~4095) +
    ((6 + sizeof(Virtio::UsedElement) * qsz + 4095) & ~4095);
}
