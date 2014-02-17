cmake_minimum_required(VERSION 2.6)
project(EbbRT)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

list(APPEND CMAKE_CXX_FLAGS
     "-std=c++0x -Wall -Werror ${CMAKE_CXX_FLAGS}")

find_package(Boost 1.53.0 REQUIRED COMPONENTS filesystem system)
include_directories(${Boost_INCLUDE_DIRS})

find_package(TBB REQUIRED)
include_directories(${TBB_INCLUDE_DIRS})

find_package(Capnp REQUIRED)
include_directories(${CAPNP_INCLUDE_DIRS})

find_package(Fdt REQUIRED)
include_directories(${FDT_INCLUDE_DIRS})

file(GLOB CapnpFiles "${CMAKE_SOURCE_DIR}/../common/src/*.capnp")
CAPNP_GENERATE_CPP(CapnpSources CapnpHeaders ${CapnpFiles})

include_directories(src/include)
include_directories(../common/src/include)
include_directories(${CMAKE_BINARY_DIR})

add_library(EbbRT 
  ${CapnpSources}
  ${CMAKE_SOURCE_DIR}/../common/src/CapnpMessage.cc
  ${CMAKE_SOURCE_DIR}/../common/src/Message.cc
  ${CMAKE_SOURCE_DIR}/src/ContextActivation.cc
  ${CMAKE_SOURCE_DIR}/src/Context.cc
  ${CMAKE_SOURCE_DIR}/src/EventManager.cc
  ${CMAKE_SOURCE_DIR}/src/GlobalIdMap.cc
  ${CMAKE_SOURCE_DIR}/src/LocalEntry.cc
  ${CMAKE_SOURCE_DIR}/src/Messenger.cc
  ${CMAKE_SOURCE_DIR}/src/NodeAllocator.cc
  ${CMAKE_SOURCE_DIR}/src/Runtime.cc
  ${CMAKE_SOURCE_DIR}/../common/src/Fdt.cc
)
