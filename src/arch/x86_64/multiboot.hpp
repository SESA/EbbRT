#ifndef EBBRT_ARCH_X86_64_MULTIBOOT_HPP
#define EBBRT_ARCH_X86_64_MULTIBOOT_HPP

#include <cstdint>

namespace ebbrt {
  class MultibootInformation {
  public:
    struct {
      uint32_t hasMem :1;
      uint32_t hasBootDevice :1;
      uint32_t hasCommandLine :1;
      uint32_t hasBootModules :1;
      uint32_t hasAOutSymbolTable :1;
      uint32_t hasElfSectionTable :1;
      uint32_t hasMemMap :1;
      uint32_t hasDrives :1;
      uint32_t hasRomConfigurationTable :1;
      uint32_t hasBootLoaderName :1;
      uint32_t hasApmTable :1;
      uint32_t hasGraphicsTable :1;
    uint32_t :20;
    };
    uint32_t memoryLower;
    uint32_t memoryHigher;
    uint32_t bootDevice;
    uint32_t commandLine;
    uint32_t modulesCount;
    uint32_t modulesAddress;
    uint32_t symbols[3];
    uint32_t memoryMapLength;
    uint32_t memoryMapAddress;
    uint32_t drivesLength;
    uint32_t drivesAddress;
    uint32_t configurationTable;
    uint32_t bootLoaderName;
    uint32_t apmTable;
    uint32_t vbeControlInfo;
    uint32_t vbeModeInfo;
    uint32_t vbeMode;
    uint32_t vbeInterfaceSegment;
    uint32_t vbeInterfaceOffset;
    uint32_t vbeInterfaceLength;
  };
  class MultibootMemoryRegion {
  public:
    uint32_t size;
    uint64_t baseAddress;
    uint64_t length;
    uint32_t type;
    static const uint32_t RAM_TYPE = 1;
  } __attribute__((packed));
};

#endif
