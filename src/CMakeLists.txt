cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
project(EbbRT CXX ASM)

# Platform Setup
if( ${CMAKE_SYSTEM_NAME} STREQUAL "EbbRT") 
  set(PLATFORM "native") 
else()
  set(PLATFORM "hosted") 
endif()
set(PLATFORM_SOURCE_DIR "${PROJECT_SOURCE_DIR}/${PLATFORM}") 

# Include platform-specific configuration
include(${PLATFORM_SOURCE_DIR}/config.cmake) 

# EbbRT Sources & Headers
file(GLOB COMMON_HEADERS    ${PROJECT_SOURCE_DIR}/*.h)
file(GLOB COMMON_SOURCES    ${PROJECT_SOURCE_DIR}/*.cc)
file(GLOB PLATFORM_HEADERS  ${PLATFORM_SOURCE_DIR}/*.h)
file(GLOB PLATFORM_SOURCES  ${PLATFORM_SOURCE_DIR}/*.cc
                            ${PLATFORM_SOURCE_DIR}/*.S)
file(GLOB CAPNP_SOURCES     ${PROJECT_SOURCE_DIR}/*.capnp)

# CapnProto 
include_directories(${CAPNP_INCLUDE_DIRS})
add_definitions(${CAPNP_DEFINITIONS})
set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CAPNPC_SRC_PREFIX ${PROJECT_SOURCE_DIR})
include_directories(${CAPNPC_OUTPUT_DIR})
CAPNP_GENERATE_CPP(CAPNP_GENERATED_SOURCES CAPNP_GENERATED_HEADERS
  ${CAPNP_SOURCES})

# EbbRT Library 
add_library(EbbRT STATIC ${COMMON_SOURCES} ${PLATFORM_SOURCES}
  ${CAPNP_GENERATED_SOURCES})

# Install
install(TARGETS EbbRT ARCHIVE DESTINATION lib
                      LIBRARY DESTINATION lib) 
install(FILES ${COMMON_HEADERS} DESTINATION include/ebbrt)
install(FILES ${PLATFORM_HEADERS} DESTINATION include/ebbrt/${PLATFORM})
install(FILES ${CAPNP_GENERATED_HEADERS} DESTINATION include/ebbrt)

# Only create cmake package for hosted runtime
if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux") 
  include(CMakePackageConfigHelpers)
  set(INSTALL_INCLUDE_DIR include/)
  set(INSTALL_LIB_DIR lib/)
  set(INSTALL_CONFIG_DIR ${INSTALL_LIB_DIR}cmake/EbbRT)
  configure_package_config_file(hosted/EbbRTConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/EbbRTConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIG_DIR} 
    PATH_VARS INSTALL_INCLUDE_DIR INSTALL_LIB_DIR 
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/EbbRTConfig.cmake
    DESTINATION ${INSTALL_CONFIG_DIR} 
  )
endif()
