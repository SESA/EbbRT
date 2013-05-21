#include <algorithm>
#include <iterator>
#include <sstream>

#include "app/HelloWorld/HelloWorld.hpp"
#include "arch/io.hpp"
#include "device/virtio_net.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"
#include "lrt/console.hpp"
#include "misc/pci.hpp"
#include "sync/compiler.hpp"

namespace {
  ebbrt::EbbRoot* construct_root()
  {
    return new ebbrt::SharedRoot<ebbrt::HelloWorldApp>;
  }
}

ebbrt::app::Config::InitEbb init_ebbs[] =
{
  {.id = ebbrt::memory_allocator,
   .create_root = ebbrt::SimpleMemoryAllocatorConstructRoot},
  {.id = ebbrt::ebb_manager,
   .create_root = ebbrt::PrimitiveEbbManagerConstructRoot},
  {.id = ebbrt::app_ebb,
   .create_root = construct_root}
};

const ebbrt::app::Config ebbrt::app::config = {
  .node_id = 0,
  .num_init = 3,
  .init_ebbs = init_ebbs
};

ebbrt::HelloWorldApp::HelloWorldApp() : lock_(ATOMIC_FLAG_INIT)
{
}

void
ebbrt::HelloWorldApp::Start()
{
  if (get_location() == 0) {
    pci::init();
    // //print out all the devices
    // std::ostringstream sstream;
    // if (!pci::devices.empty()) {
    //   std::copy(pci::devices.begin(), --(pci::devices.end()),
    //             std::ostream_iterator<pci::Device>(sstream, "============\n"));
    //   sstream << pci::devices.back();
    // }
    // const char* s = sstream.str().c_str();
    // lrt::console::write(s);

    auto it = std::find_if(pci::devices.begin(), pci::devices.end(),
                           [] (const pci::Device& d) {
                             return d.VendorId() == 0x1af4 &&
                             d.DeviceId() >= 0x1000 &&
                             d.DeviceId() <= 0x103f &&
                             d.GeneralHeaderType() &&
                             d.SubsystemId() == 1;
                           });

    if (it != pci::devices.end()) {
      std::ostringstream sstr;
      sstr << *it;
      lrt::console::write(sstr.str().c_str());
      VirtioNet vnet{*it};
      // it->BusMaster(true);
      // uint16_t addr = it->BAR0() & ~0x3;
      // uint16_t device_status = in16(addr + 18);
      std::ostringstream sstream;
      vnet.Acknowledge();
      vnet.Driver();
      sstream << std::hex << "0x" << vnet.DeviceFeatures() << std::dec << std::endl;
      sstream << std::hex << "0x" << vnet.GuestFeatures() << std::dec << std::endl;
      vnet.QueueSelect(0);
      size_t rec_size = vnet.QszBytes(vnet.QueueSize());
      void* rec_queue =  memory_allocator->memalign(4096, rec_size);
      std::memset(rec_queue, 0, rec_size);
      vnet.QueueAddress(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(rec_queue)
                                              >> 12));
      vnet.QueueSelect(1);
      size_t send_size = vnet.QszBytes(vnet.QueueSize());
      void* send_queue = memory_allocator->memalign(4096, send_size);
      std::memset(send_queue, 0, send_size);
      vnet.QueueAddress(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(send_queue)
                                              >> 12));
      Virtio::QueueDescriptor* send_descs =
        static_cast<Virtio::QueueDescriptor*>(send_queue);
      Virtio::Available* send_available =
        reinterpret_cast<Virtio::Available*>(&send_descs[vnet.QueueSize()]);
      Virtio::Used* send_used =
        reinterpret_cast<Virtio::Used*>
        ((reinterpret_cast<uintptr_t>(&send_available->ring[vnet.QueueSize()]) + 4095)
         & ~4095);
      uint8_t address[6];
      if (vnet.HasMacAddress()) {
        vnet.GetMacAddress(&address);
        sstream << std::hex;
        for (unsigned i = 0; i < 6; ++i) {
          sstream << "0x" << static_cast<int>(address[i]) << std::endl;
        }
      }
      lrt::console::write(sstream.str().c_str());
      vnet.DriverOK();
      char buffer[10];
      std::memset(buffer, 0, 10);
      char buffer2[64];
      std::memset(buffer2, 0, 64);
      buffer2[0] = 0xff;
      buffer2[1] = 0xff;
      buffer2[2] = 0xff;
      buffer2[3] = 0xff;
      buffer2[4] = 0xff;
      buffer2[5] = 0xff;
      buffer2[6] = address[0];
      buffer2[7] = address[1];
      buffer2[8] = address[2];
      buffer2[9] = address[3];
      buffer2[10] = address[4];
      buffer2[11] = address[5];
      buffer2[12] = 0x88;
      buffer2[13] = 0x12;
      std::strcpy(&buffer2[14], "Hello World!\n");
      send_descs[0].address = reinterpret_cast<uint64_t>(buffer);
      send_descs[0].length = 10;
      send_descs[0].flags.next = true;
      send_descs[0].next = 1;
      send_descs[1].address = reinterpret_cast<uint64_t>(buffer2);
      send_descs[1].length = 64;
      send_available->no_interrupt = true;
      send_available->ring[0] = 0;
      std::atomic_thread_fence(std::memory_order_release);
      send_available->index = 1;
      std::atomic_thread_fence(std::memory_order_release);
      vnet.QueueNotify(1);
      while (access_once(send_used->index) == 0)
        ;
      lrt::console::write("Send packet\n");
    }
  }
}
