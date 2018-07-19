# EbbRT native platform-specific configuration
option(__EBBRT_ENABLE_DISTRIBUTED_RUNTIME__ "Enable Distributed Runtime Support" ON)
option(__EBBRT_ENABLE_NETWORKING__ "Enable Networking" ON)
option(__EBBRT_ENABLE_BAREMETAL_NIC__ "Enable Baremetal NIC" OFF)
option(__EBBRT_ENABLE_TRACE__ "Enable Tracing Subsystem" OFF)
option(LARGE_WINDOW_HACK "Enable Large TCP Window Hack" OFF)
option(PAGE_CHECKER "Enable Page Checker" OFF)
option(VIRTIO_ZERO_COPY "Enable Virtio Zero Copy" OFF)
option(VIRTIO_NET_POLL "Enable Poll-Only VirtioNet Driver" OFF)
configure_file(${PLATFORM_SOURCE_DIR}/config.h.in config.h @ONLY)

# Build Settings
set(CMAKE_CXX_FLAGS                "-Wall -Werror -std=gnu++14 -include ${CMAKE_CURRENT_BINARY_DIR}/config.h")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g3")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
set(CMAKE_ASM_FLAGS "-DASSEMBLY")

# This is a bit of a hack to get the host capnp import path, rather than
# the sysroot ones which will be found by default
find_package(CapnProto QUIET)
get_filename_component(capnp_dir_path "${CAPNP_EXECUTABLE}" DIRECTORY)
get_filename_component(CAPNPC_IMPORT_DIRS "${capnp_dir_path}/../include" ABSOLUTE)
