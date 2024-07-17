# EbbRT hosted platform-specific configuration
set(CMAKE_CXX_FLAGS                "-Wall -Werror -std=gnu++14")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g3")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")

find_package(Boost 1.85.0 REQUIRED COMPONENTS 
  filesystem system coroutine context )
#find_package(CAPNP REQUIRED)
find_package(CapnProto REQUIRED)
find_package(TBB REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CAPNP_INCLUDE_DIRS})
include_directories(${TBB_INCLUDE_DIRS})
