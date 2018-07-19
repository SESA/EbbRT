//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_PCI_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_PCI_H_

#include <cstdint>
#include <functional>

#include <boost/optional.hpp>

namespace ebbrt {
namespace pci {
void Init();

class Function {
 public:
  Function(uint8_t bus, uint8_t device, uint8_t func);

  uint16_t GetVendorId() const;
  uint16_t GetDeviceId() const;
  uint16_t GetCommand() const;
  uint16_t GetStatus() const;
  uint8_t GetRevisionId() const;
  uint8_t GetProgIf() const;
  uint8_t GetSubclass() const;
  uint8_t GetClassCode() const;
  uint8_t GetCacheLineSize() const;
  uint8_t GetLatencyTimer() const;
  uint8_t GetHeaderType() const;
  uint8_t GetBist() const;
  uint8_t GetFunc() const;

  operator bool() const;
  bool IsMultifunc() const;
  bool IsBridge() const;

  void SetCommand(uint16_t cmd);
  void SetBusMaster(bool enable);
  void DisableInt();

  void DumpAddress() const;
  void DumpInfo() const;

 protected:
  static const constexpr uint8_t kVendorIdAddr = 0x00;
  static const constexpr uint8_t kDeviceIdAddr = 0x02;
  static const constexpr uint8_t kCommandAddr = 0x04;
  static const constexpr uint8_t kStatusAddr = 0x06;
  static const constexpr uint8_t kRevisionIdAddr = 0x08;
  static const constexpr uint8_t kProgIfAddr = 0x09;
  static const constexpr uint8_t kSubclassAddr = 0x0A;
  static const constexpr uint8_t kClassCodeAddr = 0x0B;
  static const constexpr uint8_t kCacheLineSizeAddr = 0x0C;
  static const constexpr uint8_t kLatencyTimerAddr = 0x0D;
  static const constexpr uint8_t kHeaderTypeAddr = 0x0E;
  static const constexpr uint8_t kBistAddr = 0x0F;

  static const constexpr uint16_t kCommandBusMaster = 1 << 2;
  static const constexpr uint16_t kCommandIntDisable = 1 << 10;

  static const constexpr uint8_t kHeaderMultifuncMask = 0x80;
  static const constexpr uint8_t kHeaderTypeBridge = 0x01;

  uint8_t Read8(uint8_t offset) const;
  uint16_t Read16(uint8_t offset) const;
  uint32_t Read32(uint8_t offset) const;
  void Write8(uint8_t offset, uint8_t val);
  void Write16(uint8_t offset, uint16_t val);
  void Write32(uint8_t offset, uint32_t val);

  uint8_t bus_;
  uint8_t device_;
  uint8_t func_;
};

class Device;

class Bar {
 public:
  Bar(Device& dev, uint32_t bar_val, uint8_t idx);
  ~Bar();
  bool Is64() const;
  void Map();
  uint8_t Read8(size_t offset);
  uint16_t Read16(size_t offset);
  uint32_t Read32(size_t offset);
  void Write8(size_t offset, uint8_t val);
  void Write16(size_t offset, uint16_t val);
  void Write32(size_t offset, uint32_t val);
  void* GetVaddr();

 private:
  static const constexpr uint32_t kIoSpaceFlag = 0x1;
  static const constexpr uint32_t kIoSpaceMask = ~0x3;
  static const constexpr uint32_t kMmioTypeMask = 0x6;
  static const constexpr uint32_t kMmioType32 = 0;
  static const constexpr uint32_t kMmioType64 = 0x4;
  static const constexpr uint32_t kMmioPrefetchableMask = 0x8;
  static const constexpr uint32_t kMmioMask = ~0xf;

  uint64_t addr_;
  size_t size_;
  void* vaddr_;
  bool mmio_;
  bool is_64_;
  bool prefetchable_;
};

class Device : public Function {
 public:
  Device(uint8_t bus, uint8_t device, uint8_t func);

  bool MsixEnabled() const;
  Bar& GetBar(uint8_t idx);
  bool MsixEnable();
  void MsixMaskEntry(size_t idx);
  void MsixUnmaskEntry(size_t idx);
  void SetMsixEntry(size_t entry, uint8_t vector, uint8_t dest);

 private:
  static const constexpr uint8_t kBar0Addr = 0x10;
  static const constexpr uint8_t kBar1Addr = 0x14;
  static const constexpr uint8_t kBar2Addr = 0x18;
  static const constexpr uint8_t kBar3Addr = 0x1C;
  static const constexpr uint8_t kBar4Addr = 0x20;
  static const constexpr uint8_t kBar5Addr = 0x24;
  static constexpr uint8_t BarAddr(uint8_t idx) { return kBar0Addr + idx * 4; }
  static const constexpr uint8_t kCardbusCisPtrAddr = 0x28;
  static const constexpr uint8_t kSubsystemVendorIdAddr = 0x2C;
  static const constexpr uint8_t kSubsystemIdAddr = 0x2E;
  static const constexpr uint8_t kCapabilitiesPtrAddr = 0x34;
  static const constexpr uint8_t kInterruptLineAddr = 0x3C;
  static const constexpr uint8_t kInterruptPinAddr = 0x3D;
  static const constexpr uint8_t kMinGrantAddr = 0x3E;
  static const constexpr uint8_t kMaxLatencyAddr = 0x3F;

  static const constexpr uint8_t kCapPm = 0x01;
  static const constexpr uint8_t kCapAgp = 0x02;
  static const constexpr uint8_t kCapVpd = 0x03;
  static const constexpr uint8_t kCapSlotId = 0x04;
  static const constexpr uint8_t kCapMsi = 0x05;
  static const constexpr uint8_t kCapChswp = 0x06;
  static const constexpr uint8_t kCapPcix = 0x07;
  static const constexpr uint8_t kCapHt = 0x08;
  static const constexpr uint8_t kCapVendor = 0x09;
  static const constexpr uint8_t kCapDebug = 0x0a;
  static const constexpr uint8_t kCapCres = 0x0b;
  static const constexpr uint8_t kCapHotplug = 0x0c;
  static const constexpr uint8_t kCapSubvendor = 0x0d;
  static const constexpr uint8_t kCapAgp8x = 0x0e;
  static const constexpr uint8_t kCapSecdev = 0x0f;
  static const constexpr uint8_t kCapExpress = 0x10;
  static const constexpr uint8_t kCapMsix = 0x11;
  static const constexpr uint8_t kCapSata = 0x12;
  static const constexpr uint8_t kCapPciaf = 0x13;

  static const constexpr uint8_t kMsixControl = 0x2;
  static const constexpr uint8_t kMsixTableOffset = 0x4;

  static const constexpr uint16_t kMsixControlFunctionMask = 1 << 14;
  static const constexpr uint16_t kMsixControlEnable = 1 << 15;
  static const constexpr uint16_t kMsixControlTableSizeMask = 0x7ff;

  static const constexpr uint32_t kMsixTableOffsetBirMask = 0x7;

  static const constexpr size_t kMsixTableEntrySize = 16;
  static const constexpr size_t kMsixTableEntryAddr = 0;
  static const constexpr size_t kMsixTableEntryData = 8;
  static const constexpr size_t kMsixTableEntryControl = 12;
  static const constexpr size_t kMsixTableEntryAddrLow = 0;
  static const constexpr size_t kMsixTableEntryAddrHigh = 4;

  static const constexpr uint32_t kMsixTableEntryControlMaskBit = 1;

  uint8_t FindCapability(uint8_t capability) const;
  uint32_t GetBarRaw(uint8_t idx) const;
  void SetBarRaw(uint8_t idx, uint32_t val);
  uint16_t MsixGetControl();
  void MsixSetControl(uint16_t control);

  std::array<boost::optional<Bar>, 6> bars_;
  int msix_bar_idx_;
  uint8_t msix_offset_;
  size_t msix_table_size_;
  uint32_t msix_table_offset_;

  friend class Bar;
};

void RegisterProbe(std::function<bool(Device&)> probe);

void LoadDrivers();
}  // namespace pci
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_PCI_H_
