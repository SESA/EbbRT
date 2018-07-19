//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Pci.h"

#include <cinttypes>

#include "../Align.h"
#include "../ExplicitlyConstructed.h"
#include "Debug.h"
#include "GeneralPurposeAllocator.h"
#include "Io.h"
#include "VMem.h"
#include "VMemAllocator.h"

namespace {

const constexpr uint32_t kPciAddressPort = 0xCF8;
const constexpr uint32_t kPciDataPort = 0xCFC;
const constexpr uint32_t kPciAddressEnable = 0x80000000;
const constexpr uint8_t kPciBusOffset = 16;
const constexpr uint8_t kPciDeviceOffset = 11;
const constexpr uint8_t kPciFuncOffset = 8;

void PciSetAddr(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
  ebbrt::io::Out32(kPciAddressPort, kPciAddressEnable | (bus << kPciBusOffset) |
                                        (device << kPciDeviceOffset) |
                                        (func << kPciFuncOffset) | offset);
}

uint8_t PciRead8(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
  PciSetAddr(bus, device, func, offset);
  return ebbrt::io::In8(kPciDataPort + (offset & 3));
}
uint16_t PciRead16(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
  PciSetAddr(bus, device, func, offset);
#ifdef __EBBRT_ENABLE_BAREMETAL_NIC__
  return ebbrt::io::In16(kPciDataPort + (offset & 2));
#else
  return ebbrt::io::In16(kPciDataPort);
#endif
}

uint32_t PciRead32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
  PciSetAddr(bus, device, func, offset);
  return ebbrt::io::In32(kPciDataPort);
}

void PciWrite16(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset,
                uint16_t val) {
  PciSetAddr(bus, device, func, offset);

#ifdef __EBBRT_ENABLE_BAREMETAL_NIC__
  ebbrt::io::Out16(kPciDataPort + (offset & 2), val);
#else
  ebbrt::io::Out16(kPciDataPort, val);
#endif
}

void PciWrite32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset,
                uint32_t val) {
  PciSetAddr(bus, device, func, offset);
  ebbrt::io::Out32(kPciDataPort, val);
}

ebbrt::ExplicitlyConstructed<std::vector<ebbrt::pci::Device>> devices;
ebbrt::ExplicitlyConstructed<
    std::vector<std::function<bool(ebbrt::pci::Device&)>>>
    driver_probes;

void EnumerateBus(uint8_t bus) {
  for (auto device = 0; device < 32; ++device) {
    auto dev = ebbrt::pci::Function(bus, device, 0);
    if (dev)
      continue;

    auto nfuncs = dev.IsMultifunc() ? 8 : 1;
    for (auto func = 0; func < nfuncs; ++func) {
      dev = ebbrt::pci::Function(bus, device, func);
      if (dev)
        continue;

      dev.DumpAddress();
      dev.DumpInfo();

      if (dev.IsBridge()) {
        // ebbrt::kabort("Secondary bus unsupported!\n");
        continue;
      } else {
        devices->emplace_back(bus, device, func);
      }
    }
  }
}

void EnumerateAllBuses() {
  auto bus = ebbrt::pci::Function(0, 0, 0);
  if (!bus.IsMultifunc()) {
    // single bus
    EnumerateBus(0);
  } else {
    // potentially multiple buses
    for (auto func = 0; func < 8; ++func) {
      bus = ebbrt::pci::Function(0, 0, func);
      if (!bus)
        continue;
      EnumerateBus(func);
    }
  }
}
}  // namespace

void ebbrt::pci::Init() {
  devices.construct();
  driver_probes.construct();
  EnumerateAllBuses();
#ifdef __EBBRT_ENABLE_BAREMETAL_NIC__
  // TODO - Kludge to identify where NIC sits in device tree, should incorporate
  // Dan's pull request for enumerating bridges
  EnumerateBus(0x1);
#endif
}

void ebbrt::pci::RegisterProbe(std::function<bool(pci::Device&)> probe) {
  driver_probes->emplace_back(std::move(probe));
}

void ebbrt::pci::LoadDrivers() {
  for (auto& dev : *devices) {
    for (auto& probe : *driver_probes) {
      if (probe(dev))
        break;
    }
  }
}

ebbrt::pci::Function::Function(uint8_t bus, uint8_t device, uint8_t func)
    : bus_(bus), device_(device), func_(func) {}

uint8_t ebbrt::pci::Function::Read8(uint8_t offset) const {
  return PciRead8(bus_, device_, func_, offset);
}

uint16_t ebbrt::pci::Function::Read16(uint8_t offset) const {
  return PciRead16(bus_, device_, func_, offset);
}

uint32_t ebbrt::pci::Function::Read32(uint8_t offset) const {
  return PciRead32(bus_, device_, func_, offset);
}

void ebbrt::pci::Function::Write16(uint8_t offset, uint16_t val) {
  return PciWrite16(bus_, device_, func_, offset, val);
}

void ebbrt::pci::Function::Write32(uint8_t offset, uint32_t val) {
  return PciWrite32(bus_, device_, func_, offset, val);
}

uint16_t ebbrt::pci::Function::GetVendorId() const {
  return Read16(kVendorIdAddr);
}
uint16_t ebbrt::pci::Function::GetDeviceId() const {
  return Read16(kDeviceIdAddr);
}
uint16_t ebbrt::pci::Function::GetCommand() const {
  return Read16(kCommandAddr);
}

uint8_t ebbrt::pci::Function::GetClassCode() const {
  return Read8(kClassCodeAddr);
}

uint8_t ebbrt::pci::Function::GetFunc() const { return func_; }

uint8_t ebbrt::pci::Function::GetSubclass() const {
  return Read8(kSubclassAddr);
}

uint8_t ebbrt::pci::Function::GetProgIf() const { return Read8(kProgIfAddr); }

uint8_t ebbrt::pci::Function::GetHeaderType() const {
  return Read8(kHeaderTypeAddr) & ~kHeaderMultifuncMask;
}

ebbrt::pci::Function::operator bool() const { return GetVendorId() == 0xffff; }

bool ebbrt::pci::Function::IsMultifunc() const {
  return Read8(kHeaderTypeAddr) & kHeaderMultifuncMask;
}

bool ebbrt::pci::Function::IsBridge() const {
  return GetHeaderType() == kHeaderTypeBridge;
}

void ebbrt::pci::Function::SetCommand(uint16_t cmd) {
  Write16(kCommandAddr, cmd);
}

void ebbrt::pci::Function::SetBusMaster(bool enable) {
  auto cmd = GetCommand();
  if (enable) {
    cmd |= kCommandBusMaster;
  } else {
    cmd &= ~kCommandBusMaster;
  }
  SetCommand(cmd);
}

void ebbrt::pci::Function::DisableInt() {
  auto cmd = GetCommand();
  cmd |= kCommandIntDisable;
  SetCommand(cmd);
}

void ebbrt::pci::Function::DumpAddress() const {
  kprintf("%u:%u:%u\n", bus_, device_, func_);
}

void ebbrt::pci::Function::DumpInfo() const {
  kprintf("Vendor ID: 0x%x  ", GetVendorId());
  kprintf("Device ID: 0x%x\n", GetDeviceId());
}

ebbrt::pci::Bar::Bar(pci::Device& dev, uint32_t bar_val, uint8_t idx)
    : vaddr_(nullptr), is_64_(false), prefetchable_(false) {
  mmio_ = !(bar_val & kIoSpaceFlag);
  if (mmio_) {
    addr_ = bar_val & kMmioMask;
    is_64_ = (bar_val & kMmioTypeMask) == kMmioType64;
    prefetchable_ = bar_val & kMmioPrefetchableMask;

    dev.SetBarRaw(idx, 0xffffffff);
    auto low_sz = dev.GetBarRaw(idx) & kMmioMask;
    dev.SetBarRaw(idx, bar_val);

    uint32_t high_sz = ~0;
    if (is_64_) {
      auto bar_val2 = dev.GetBarRaw(idx + 1);
      addr_ |= static_cast<uint64_t>(bar_val2) << 32;
      dev.SetBarRaw(idx + 1, 0xffffffff);
      high_sz = dev.GetBarRaw(idx + 1);
      dev.SetBarRaw(idx + 1, bar_val2);
    }
    uint64_t sz = static_cast<uint64_t>(high_sz) << 32 | low_sz;
    size_ = ~sz + 1;
  } else {
    addr_ = bar_val & kIoSpaceMask;
    dev.SetBarRaw(idx, 0xffffffff);
    auto sz = dev.GetBarRaw(idx) & kIoSpaceMask;
    dev.SetBarRaw(idx, bar_val);

    size_ = ~sz + 1;
  }

  kprintf("PCI: %u:%u:%u - BAR%u %#018" PRIx64 " - %#018" PRIx64 "\n", dev.bus_,
          dev.device_, dev.func_, idx, addr_, addr_ + size_);
}

ebbrt::pci::Bar::~Bar() {
  kbugon(vaddr_ != nullptr, "pci::Bar: Need to free mapped region\n");
}

void* ebbrt::pci::Bar::GetVaddr() { return vaddr_; }

bool ebbrt::pci::Bar::Is64() const { return is_64_; }

void ebbrt::pci::Bar::Map() {
  if (!mmio_)
    return;

  auto npages = align::Up(size_, pmem::kPageSize) >> pmem::kPageShift;

#ifdef __EBBRT_ENABLE_BAREMETAL_NIC__
  auto pf = std::make_unique<MulticorePciFaultHandler>();
  auto& ref = *pf;
  auto page = vmem_allocator->Alloc(npages, std::move(pf));
  vaddr_ = reinterpret_cast<void*>(page.ToAddr());
  kbugon(page == Pfn::None(), "Failed to allocate virtual pages for mmio\n");
  vmem::MapMemory(page, Pfn::Down(addr_), size_);
  ref.SetMap(page, Pfn::Down(addr_), size_);
#else
  auto page = vmem_allocator->Alloc(npages);
  vaddr_ = reinterpret_cast<void*>(page.ToAddr());
  kbugon(page == Pfn::None(), "Failed to allocate virtual pages for mmio\n");
  vmem::MapMemory(page, Pfn::Down(addr_), size_);
#endif
}

uint8_t ebbrt::pci::Bar::Read8(size_t offset) {
  if (mmio_) {
    auto addr = static_cast<void*>(static_cast<char*>(vaddr_) + offset);
    return *static_cast<volatile uint8_t*>(addr);
  } else {
    return io::In8(addr_ + offset);
  }
}

uint16_t ebbrt::pci::Bar::Read16(size_t offset) {
  if (mmio_) {
    auto addr = static_cast<void*>(static_cast<char*>(vaddr_) + offset);
    return *static_cast<volatile uint16_t*>(addr);
  } else {
    return io::In16(addr_ + offset);
  }
}

uint32_t ebbrt::pci::Bar::Read32(size_t offset) {
  if (mmio_) {
    auto addr = static_cast<void*>(static_cast<char*>(vaddr_) + offset);
    return *static_cast<volatile uint32_t*>(addr);
  } else {
    return io::In32(addr_ + offset);
  }
}

void ebbrt::pci::Bar::Write8(size_t offset, uint8_t val) {
  if (mmio_) {
    auto addr = static_cast<void*>(static_cast<char*>(vaddr_) + offset);
    *static_cast<volatile uint8_t*>(addr) = val;
  } else {
    io::Out8(addr_ + offset, val);
  }
}

void ebbrt::pci::Bar::Write16(size_t offset, uint16_t val) {
  if (mmio_) {
    auto addr = static_cast<void*>(static_cast<char*>(vaddr_) + offset);
    *static_cast<volatile uint16_t*>(addr) = val;
  } else {
    io::Out16(addr_ + offset, val);
  }
}

void ebbrt::pci::Bar::Write32(size_t offset, uint32_t val) {
  if (mmio_) {
    auto addr = static_cast<void*>(static_cast<char*>(vaddr_) + offset);
    *static_cast<volatile uint32_t*>(addr) = val;
  } else {
    io::Out32(addr_ + offset, val);
  }
}

ebbrt::pci::Device::Device(uint8_t bus, uint8_t device, uint8_t func)
    : Function(bus, device, func), msix_bar_idx_(-1) {
  for (auto idx = 0; idx <= 6; ++idx) {
    auto bar = Read32(BarAddr(idx));
    if (bar == 0)
      continue;

    bars_[idx] = Bar(*this, bar, idx);

    if (bars_[idx]->Is64())
      idx++;
  }
}

uint8_t ebbrt::pci::Device::FindCapability(uint8_t capability) const {
  auto ptr = Read8(kCapabilitiesPtrAddr);
  while (ptr != 0) {
    auto cap = Read8(ptr);
    if (cap == capability)
      return ptr;

    ptr = Read8(ptr + 1);
  }

  return 0xFF;
}

uint32_t ebbrt::pci::Device::GetBarRaw(uint8_t idx) const {
  return Read32(BarAddr(idx));
}

void ebbrt::pci::Device::SetBarRaw(uint8_t idx, uint32_t val) {
  Write32(BarAddr(idx), val);
}

uint16_t ebbrt::pci::Device::MsixGetControl() {
  return Read16(msix_offset_ + kMsixControl);
}

void ebbrt::pci::Device::MsixSetControl(uint16_t control) {
  Write16(msix_offset_ + kMsixControl, control);
}

bool ebbrt::pci::Device::MsixEnabled() const { return msix_bar_idx_ != -1; }

ebbrt::pci::Bar& ebbrt::pci::Device::GetBar(uint8_t idx) {
  if (!bars_[idx])
    throw std::runtime_error("BAR does not exist\n");

  return *bars_[idx];
}

bool ebbrt::pci::Device::MsixEnable() {
  auto offset = FindCapability(kCapMsix);
  if (offset == 0xFF)
    return false;

  msix_offset_ = offset;

  auto control = MsixGetControl();
  msix_table_size_ = (control & kMsixControlTableSizeMask) + 1;
  auto table_offset_val = Read32(offset + kMsixTableOffset);
  msix_bar_idx_ = table_offset_val & kMsixTableOffsetBirMask;
  msix_table_offset_ = table_offset_val & ~kMsixTableOffsetBirMask;

  kprintf("MSIX - %u entries at BAR%u:%lx\n", msix_table_size_, msix_bar_idx_,
          msix_table_offset_);

  auto& msix_bar = GetBar(msix_bar_idx_);
  msix_bar.Map();

  DisableInt();

  auto ctrl = MsixGetControl();
  ctrl |= kMsixControlEnable;
  ctrl |= kMsixControlFunctionMask;
  MsixSetControl(ctrl);

  for (size_t i = 0; i < msix_table_size_; ++i) {
    MsixMaskEntry(i);
  }

  ctrl &= ~kMsixControlFunctionMask;
  MsixSetControl(ctrl);

  return true;
}

void ebbrt::pci::Device::MsixMaskEntry(size_t idx) {
  if (!MsixEnabled())
    return;

  if (idx >= msix_table_size_)
    return;

  auto& msix_bar = GetBar(msix_bar_idx_);
  auto offset =
      msix_table_offset_ + idx * kMsixTableEntrySize + kMsixTableEntryControl;
  auto ctrl = msix_bar.Read32(offset);
  ctrl |= kMsixTableEntryControlMaskBit;
  msix_bar.Write32(offset, ctrl);
}

void ebbrt::pci::Device::MsixUnmaskEntry(size_t idx) {
  if (!MsixEnabled())
    return;

  if (idx >= msix_table_size_)
    return;

  auto& msix_bar = GetBar(msix_bar_idx_);
  auto offset =
      msix_table_offset_ + idx * kMsixTableEntrySize + kMsixTableEntryControl;
  auto ctrl = msix_bar.Read32(offset);
  ctrl &= ~kMsixTableEntryControlMaskBit;
  msix_bar.Write32(offset, ctrl);
}

void ebbrt::pci::Device::SetMsixEntry(size_t entry, uint8_t vector,
                                      uint8_t dest) {
  auto& msix_bar = GetBar(msix_bar_idx_);
  auto offset = msix_table_offset_ + entry * kMsixTableEntrySize;

#ifdef __EBBRT_ENABLE_BAREMETAL_NIC__
  // more precise
  msix_bar.Write32(offset + kMsixTableEntryAddrLow, 0xFEE00000 | dest << 12);
  msix_bar.Write32(offset + kMsixTableEntryAddrHigh, 0x0);
#else
  msix_bar.Write32(offset + kMsixTableEntryAddr, 0xFEE00000 | dest << 12);
#endif

  msix_bar.Write32(offset + kMsixTableEntryData, vector);
  MsixUnmaskEntry(entry);
}
