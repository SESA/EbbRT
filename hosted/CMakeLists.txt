project(EbbRT_Linux CXX)
cmake_minimum_required(VERSION 2.6)
set(VERSION 0.1-dev)

include(CMakePackageConfigHelpers)
include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-std=gnu++14" HAS_CXX14)
if(NOT HAS_CXX14)
  message( FATAL_ERROR "Compiler does not support -std=gnu++14")
endif()

add_compile_options(-std=gnu++14 -Wall -Werror)

# Dependencies ======================================================

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")

find_package(Boost 1.53.0 REQUIRED COMPONENTS 
  filesystem system coroutine context )
find_package(Fdt REQUIRED)
find_package(Capnp REQUIRED)
find_package(TBB REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CAPNP_INCLUDE_DIRS})
include_directories(${FDT_INCLUDE_DIRS})
include_directories(${TBB_INCLUDE_DIRS})

# Source ============================================================

file(GLOB CapnpFiles "${PROJECT_SOURCE_DIR}/../common/src/*.capnp")
add_definitions(${CAPNP_DEFINITIONS})
set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CAPNPC_SRC_PREFIX "${CMAKE_SOURCE_DIR}/../common/src")
include_directories(${CAPNPC_OUTPUT_DIR})
CAPNP_GENERATE_CPP(CapnpSources CapnpHeaders ${CapnpFiles})

set(EBBRT_SOURCE_DIRS 
  ${PROJECT_SOURCE_DIR}/src/include 
  ${PROJECT_SOURCE_DIR}/../common/src/ # capnp.h files
  ${PROJECT_SOURCE_DIR}/../common/src/include 
)

include_directories(${EBBRT_SOURCE_DIRS})

add_library(EbbRT 
  ${CapnpSources}
  ${PROJECT_SOURCE_DIR}/../common/src/CapnpMessage.cc
  ${PROJECT_SOURCE_DIR}/../common/src/EbbAllocator.cc
  ${PROJECT_SOURCE_DIR}/../common/src/Fdt.cc
  ${PROJECT_SOURCE_DIR}/../common/src/IOBuf.cc
  ${PROJECT_SOURCE_DIR}/../common/src/IOBufRef.cc
  ${PROJECT_SOURCE_DIR}/../common/src/Message.cc
  ${PROJECT_SOURCE_DIR}/../common/src/UniqueIOBuf.cc
  ${PROJECT_SOURCE_DIR}/../common/src/StaticIOBuf.cc
  ${PROJECT_SOURCE_DIR}/../common/src/SharedIOBufRef.cc
  ${PROJECT_SOURCE_DIR}/src/Clock.cc
  ${PROJECT_SOURCE_DIR}/src/ContextActivation.cc
  ${PROJECT_SOURCE_DIR}/src/Context.cc
  ${PROJECT_SOURCE_DIR}/src/Debug.cc
  ${PROJECT_SOURCE_DIR}/src/EventManager.cc
  ${PROJECT_SOURCE_DIR}/src/GlobalIdMap.cc
  ${PROJECT_SOURCE_DIR}/src/LocalEntry.cc
  ${PROJECT_SOURCE_DIR}/src/LocalIdMap.cc
  ${PROJECT_SOURCE_DIR}/src/Messenger.cc
  ${PROJECT_SOURCE_DIR}/src/NodeAllocator.cc
  ${PROJECT_SOURCE_DIR}/src/Runtime.cc
  ${PROJECT_SOURCE_DIR}/src/Console.cc
  ${PROJECT_SOURCE_DIR}/src/Timer.cc
)

# Install  ===========================================================

set(INSTALL_INCLUDE_DIR include/)
set(INSTALL_LIB_DIR lib/)
set(INSTALL_CONFIG_DIR ${INSTALL_LIB_DIR}cmake/EbbRT)

install(TARGETS EbbRT 
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  ) 

# install headers
foreach( D ${EBBRT_SOURCE_DIRS} )
  file(GLOB_RECURSE H "${D}/*.h")
  install(FILES ${H} DESTINATION include/ebbrt)
endforeach()

# EbbRTConfig.cmake 
configure_package_config_file(EbbRTConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/EbbRTConfig.cmake
  INSTALL_DESTINATION ${INSTALL_CONFIG_DIR} 
  PATH_VARS INSTALL_INCLUDE_DIR INSTALL_LIB_DIR 
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/EbbRTConfig.cmake
  DESTINATION ${INSTALL_CONFIG_DIR} 
)
