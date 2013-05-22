#include <algorithm>

#include "device/virtio.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/Ethernet/VirtioNet.hpp"
#include "ebb/MemoryAllocator/MemoryAllocator.hpp"

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

ebbrt::EbbRoot*
ebbrt::VirtioNet::ConstructRoot()
{
  return new SharedRoot<VirtioNet>();
}

ebbrt::VirtioNet::VirtioNet()
{
  pci::init();

  auto it = std::find_if(pci::devices.begin(), pci::devices.end(),
                         [] (const pci::Device& d) {
                           return d.VendorId() == 0x1af4 &&
                           d.DeviceId() >= 0x1000 &&
                           d.DeviceId() <= 0x103f &&
                           d.GeneralHeaderType() &&
                           d.SubsystemId() == 1;
                         });

  if (it == pci::devices.end()) {
    //FIXME: Throw an exception!
  }

  io_addr_ = static_cast<uint16_t>(it->BAR0() & ~0x3);
  it->BusMaster(true);

  virtio::acknowledge(io_addr_);
  virtio::driver(io_addr_);

  // Negotiate features
  Features features;
  features.raw = virtio::device_features(io_addr_);

  Features supported_features;
  supported_features.raw = 0;
  supported_features.mac = features.mac;
  virtio::guest_features(io_addr_, supported_features.raw);

  //TODO: Initialize Receive

  // Initialize Send
  virtio::queue_select(io_addr_, 1);
  send_max_ = virtio::queue_size(io_addr_);
  size_t send_size = virtio::qsz_bytes(send_max_);
  void* send_queue = memory_allocator->memalign(4096, send_size);
  std::memset(send_queue, 0, send_size);
  virtio::queue_address(io_addr_, static_cast<uint32_t>
                        (reinterpret_cast<uintptr_t>(send_queue) >> 12));
  send_descs_ = static_cast<virtio::QueueDescriptor*>(send_queue);
  send_available_ = reinterpret_cast<virtio::Available*>
    (&send_descs_[send_max_]);
  send_used_ = reinterpret_cast<virtio::Used*>
    ((reinterpret_cast<uintptr_t>(&send_available_->ring[send_max_]) + 4095)
     & ~4095);

  // Tell the device we are good to go
  virtio::driver_ok(io_addr_);
}

void*
ebbrt::VirtioNet::Allocate(size_t size)
{
  return nullptr;
}
