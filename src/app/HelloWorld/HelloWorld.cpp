#include <algorithm>
#include <iterator>
#include <sstream>

#include "app/HelloWorld/HelloWorld.hpp"
#include "device/virtio.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/EbbAllocator/PrimitiveEbbAllocator.hpp"
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
  {.id = ebbrt::ebb_allocator,
   .create_root = ebbrt::PrimitiveEbbAllocatorConstructRoot},
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
      VirtioHeader* header =
        new (reinterpret_cast<void*>(it->BAR1() & ~0xf)) VirtioHeader;
      VirtioHeader::DeviceStatus status;
      status.raw = 0;
      status.acknowledge = 1;
      access_once(header->device_status.raw) = status.raw;
      status.driver = 1;
      access_once(header->device_status.raw) = status.raw;
      std::ostringstream sstream;
      sstream << *header;
      const char* s = sstream.str().c_str();
      lrt::console::write(s);
    }
  }
}
