#include "device/virtio.hpp"

std::ostream&
ebbrt::operator<<(std::ostream& os, const VirtioHeader& header)
{
  os << std::hex;
  os << "HW features: " << header.hw_features.raw << std::endl;
  os << "OS features: " << header.os_features.raw << std::endl;
  os << "Queue address: " << header.queue_address << std::endl;
  os << "Queue size: " << header.queue_size << std::endl;
  os << "Queue select: " << header.queue_select << std::endl;
  os << "Queue notify: " << header.queue_notify << std::endl;
  os << "Device status: " << static_cast<int>(header.device_status.raw) <<
    std::endl;
  os << "ISR status: " << static_cast<int>(header.isr_status) << std::endl;
  return os;
}
