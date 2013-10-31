#include <memory>

#include "app/app.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "ebb/EbbManager/EbbManager.hpp"
#include "ebb/Config/Fdt.hpp"

#ifdef __ebbrt__
#include "ebb/Ethernet/VirtioNet.hpp"
#include "ebb/PCI/PCI.hpp"
#include "app/Sage/Matrix.hpp"
#include "lrt/bare/boot.hpp"
#endif

#ifdef __linux__
#include "ebbrt.hpp"
#include "app/Sage/ebb_matrix_helper.hpp"
#include "ebb/NodeAllocator/Kittyhawk.hpp"
#include "ebb/Config/Config.hpp"
#endif

#ifdef __linux__
constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  { .name = "MemoryAllocator", .id = 1 }, { .name = "EbbManager", .id = 2 },
  { .name = "Gthread", .id = 3 },         { .name = "Syscall", .id = 4 },
  { .name = "EventManager", .id = 5 },    { .name = "MessageManager", .id = 7 },
  { .name = "Network", .id = 9 },         { .name = "Timer", .id = 10 },
};

const ebbrt::app::Config ebbrt::app::config = {
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};

std::unique_ptr<ebbrt::EbbRT> instance;
std::unique_ptr<ebbrt::Context> context;
bool initialized = false;

void activate_context() {
  if (!initialized) {
    int len;
    auto dtb = getenv("SAGE_DTB");
    assert(dtb != nullptr);
    auto fdt = ebbrt::app::LoadFile(dtb, &len);
    instance.reset(new ebbrt::EbbRT(fdt));
    context.reset(new ebbrt::Context(*instance));
    context->Activate();

    ebbrt::node_allocator =
        ebbrt::EbbRef<ebbrt::NodeAllocator>(ebbrt::ebb_manager->AllocateId());
    ebbrt::ebb_manager->Bind(ebbrt::Kittyhawk::ConstructRoot,
                             ebbrt::node_allocator);

    ebbrt::config_handle =
        ebbrt::EbbRef<ebbrt::Config>(ebbrt::ebb_manager->AllocateId());
    ebbrt::ebb_manager->Bind(ebbrt::Fdt::ConstructRoot, ebbrt::config_handle);

    auto mutable_fdt = ebbrt::app::LoadFile(dtb, &len);
    ebbrt::config_handle->SetConfig(mutable_fdt);
    // TODO: find this out programmatically?
    ebbrt::config_handle->SetString("/", "frontend_ip", "10.255.0.1");
    ebbrt::config_handle->SetInt32("/", "space_id", 1);

    ebbrt::message_manager->StartListening();

    initialized = true;
  } else {
    context->Activate();
  }
}

void deactivate_context() { context->Deactivate(); }

#else

void ebbrt::app::start() {
  pci = EbbRef<PCI>(ebb_manager->AllocateId());
  ebb_manager->Bind(PCI::ConstructRoot, pci);

  ethernet = EbbRef<Ethernet>(ebb_manager->AllocateId());
  ebb_manager->Bind(VirtioNet::ConstructRoot, ethernet);

  ebbrt::config_handle =
    ebbrt::EbbRef<ebbrt::Config>(ebbrt::ebb_manager->AllocateId());
  ebbrt::ebb_manager->Bind(ebbrt::Fdt::ConstructRoot, ebbrt::config_handle);
  config_handle->SetConfig(lrt::boot::fdt);

  // FIXME: get from config
  message_manager->StartListening();

  auto id = ebbrt::config_handle->GetInt32("/ebbs/Matrix", "id");
  auto matrix = EbbRef<Matrix>(id);
  matrix->Connect();
}
#endif
