cmake_minimum_required(VERSION 2.6)

# This must be before the project() command so cmake finds our platform
# definition
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# This skips compiler verification which will fail because we haven't finished
# installing the necessary libraries
set(CMAKE_C_COMPILER_FORCED 1)
set(CMAKE_CXX_COMPILER_FORCED 1)

project(EbbRT CXX ASM)

### Options ###

option(__EBBRT_ENABLE_FDT__ "Enable Flattened Device Tree Support" ON) 
option(__EBBRT_ENABLE_DISTRIBUTED_RUNTIME__ "Enable Distributed Runtime Support" ON)
option(__EBBRT_ENABLE_NETWORKING__ "Enable Networking" ON)
option(__EBBRT_ENABLE_STATIC_IP__ "Enable Static IP Address Configuration" OFF)
option(__EBBRT_ENABLE_TRACE__ "Enable Tracing Subsystem" OFF)
option(LARGE_WINDOW_HACK "Enable Large TCP Window Hack" OFF)
option(PAGE_CHECKER "Enable Page Checker" OFF)
option(VIRTIO_ZERO_COPY "Enable Virtio Zero Copy" OFF)
option(VIRTIO_NET_POLL "Enable Poll-Only VirtioNet Driver" OFF)

configure_file(config.h.in config.h @ONLY)

### Definitions for the EbbRT library ###

# Initialize CXXFLAGS.
set(CMAKE_CXX_FLAGS                "-Wall -Werror -std=gnu++14 -include ${CMAKE_CURRENT_BINARY_DIR}/config.h")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g3")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
set(CMAKE_ASM_FLAGS "-DASSEMBLY")

### Sources ###

set(sources
  ${CMAKE_SOURCE_DIR}/src/VMem.cc
  ${CMAKE_SOURCE_DIR}/src/Apic.cc
  ${CMAKE_SOURCE_DIR}/src/NetEth.cc
  ${CMAKE_SOURCE_DIR}/src/Idt.cc
  ${CMAKE_SOURCE_DIR}/src/Trace.cc
  ${CMAKE_SOURCE_DIR}/src/BootFdt.cc
  ${CMAKE_SOURCE_DIR}/src/NetIcmp.cc
  ${CMAKE_SOURCE_DIR}/src/MemMap.cc
  ${CMAKE_SOURCE_DIR}/src/Cpu.cc
  ${CMAKE_SOURCE_DIR}/src/NetUdp.cc
  ${CMAKE_SOURCE_DIR}/src/Acpi.cc
  ${CMAKE_SOURCE_DIR}/src/Perf.cc
  ${CMAKE_SOURCE_DIR}/src/Smp.cc
  ${CMAKE_SOURCE_DIR}/src/LocalIdMap.cc
  ${CMAKE_SOURCE_DIR}/src/Timer.cc
  ${CMAKE_SOURCE_DIR}/src/Net.cc
  ${CMAKE_SOURCE_DIR}/src/NetIp.cc
  ${CMAKE_SOURCE_DIR}/src/Numa.cc
  ${CMAKE_SOURCE_DIR}/src/NetChecksum.cc
  ${CMAKE_SOURCE_DIR}/src/Runtime.cc
  ${CMAKE_SOURCE_DIR}/src/VMemAllocator.cc
  ${CMAKE_SOURCE_DIR}/src/Trans.cc
  ${CMAKE_SOURCE_DIR}/src/EventManager.cc
  ${CMAKE_SOURCE_DIR}/src/Messenger.cc
  ${CMAKE_SOURCE_DIR}/src/Uart8250.cc
  ${CMAKE_SOURCE_DIR}/src/Clock.cc
  ${CMAKE_SOURCE_DIR}/src/Main.cc
  ${CMAKE_SOURCE_DIR}/src/Random.cc
  ${CMAKE_SOURCE_DIR}/src/NetDhcp.cc
  ${CMAKE_SOURCE_DIR}/src/Multiboot.cc
  ${CMAKE_SOURCE_DIR}/src/Gthread.cc
  ${CMAKE_SOURCE_DIR}/src/SlabAllocator.cc
  ${CMAKE_SOURCE_DIR}/src/Newlib.cc
  ${CMAKE_SOURCE_DIR}/src/VirtioNet.cc
  ${CMAKE_SOURCE_DIR}/src/Debug.cc
  ${CMAKE_SOURCE_DIR}/src/Pic.cc
  ${CMAKE_SOURCE_DIR}/src/EarlyPageAllocator.cc
  ${CMAKE_SOURCE_DIR}/src/PitClock.cc
  ${CMAKE_SOURCE_DIR}/src/Cpuid.cc
  ${CMAKE_SOURCE_DIR}/src/GlobalIdMap.cc
  ${CMAKE_SOURCE_DIR}/src/NetTcp.cc
  ${CMAKE_SOURCE_DIR}/src/PvClock.cc
  ${CMAKE_SOURCE_DIR}/src/Pci.cc
  ${CMAKE_SOURCE_DIR}/src/LocalEntry.cc
  ${CMAKE_SOURCE_DIR}/src/E820.cc
  ${CMAKE_SOURCE_DIR}/src/Tls.cc
  ${CMAKE_SOURCE_DIR}/src/PageAllocator.cc
  ${CMAKE_SOURCE_DIR}/src/Memset.S
  ${CMAKE_SOURCE_DIR}/src/Isr.S
  ${CMAKE_SOURCE_DIR}/src/Boot.S
  ${CMAKE_SOURCE_DIR}/src/Memchr.S
  ${CMAKE_SOURCE_DIR}/src/Stack.S
  ${CMAKE_SOURCE_DIR}/src/Memcpy.S
  ${CMAKE_SOURCE_DIR}/../common/src/Buffer.cc
  ${CMAKE_SOURCE_DIR}/../common/src/StaticIOBuf.cc
  ${CMAKE_SOURCE_DIR}/../common/src/UniqueIOBuf.cc
  ${CMAKE_SOURCE_DIR}/../common/src/SharedIOBufRef.cc
  ${CMAKE_SOURCE_DIR}/../common/src/IOBuf.cc
  ${CMAKE_SOURCE_DIR}/../common/src/Fdt.cc
  ${CMAKE_SOURCE_DIR}/../common/src/IOBufRef.cc
  ${CMAKE_SOURCE_DIR}/../common/src/Message.cc
  ${CMAKE_SOURCE_DIR}/../common/src/EbbAllocator.cc
  ${CMAKE_SOURCE_DIR}/../common/src/CapnpMessage.cc
  )

set(capnp_sources
  ${CMAKE_SOURCE_DIR}/../common/src/GlobalIdMessage.capnp
  ${CMAKE_SOURCE_DIR}/../common/src/RuntimeInfo.capnp
  )

# Initialize capnproto

# This is a bit of a hack to get the host capnp import path, rather than
# the sysroot ones which will be found by default
find_package(CapnProto QUIET)
get_filename_component(capnp_dir_path "${CAPNP_EXECUTABLE}" DIRECTORY)
get_filename_component(CAPNPC_IMPORT_DIRS "${capnp_dir_path}/../include" ABSOLUTE)
find_package(CapnProto REQUIRED)

include_directories(${CAPNP_INCLUDE_DIRS})
add_definitions(${CAPNP_DEFINITIONS})
set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CAPNPC_SRC_PREFIX "${CMAKE_SOURCE_DIR}/../common/src")
include_directories(${CAPNPC_OUTPUT_DIR})
capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS ${capnp_sources})

add_library(ebbrt STATIC ${CAPNP_SRCS} ${sources})

install(TARGETS ebbrt
  ARCHIVE DESTINATION lib)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/include/ebbrt DESTINATION include
  FILES_MATCHING PATTERN "*.h")

install(DIRECTORY ${CMAKE_SOURCE_DIR}/../common/src/include/ebbrt DESTINATION include
  FILES_MATCHING PATTERN "*.h")

install(FILES ${CAPNP_HDRS} DESTINATION include/ebbrt)
