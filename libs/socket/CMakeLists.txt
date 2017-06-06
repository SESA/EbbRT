project("ebbrt-socket")
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
include(CMakePackageConfigHelpers)
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g3")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")
add_compile_options(-Wall -Werror)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable -Wno-unused-function")

# Platform-specific package 
if( ${CMAKE_SYSTEM_NAME} STREQUAL "EbbRT")
  message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
else()
  message(WARNING "System name unsupported: ${CMAKE_SYSTEM_NAME}")
  return()
endif()

# Library targets
set(LIB_SOURCES 
  SocketApi.cc 
  SocketManager.cc
  Vfs.cc
  lwip/api/netdb.c
  lwip/core/def.c
  lwip/core/ip_addr.c
  lwip/core/memp.c
  lwip/inet/inet_ntop.c
  lwip/inet/inet_pton.c
)
set(LIB_INCLUDES
  gnu/include
  lwip/include
  lwip/include/posix
)
include_directories(${LIB_INCLUDES})
add_library(ebbrt-socket ${LIB_SOURCES} )

# Installation directories 
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH
  "Installation directory for header files")
set(INSTALL_LWIP_INCLUDE_DIR include/ebbrt-socket/lwip/include CACHE PATH
  "Installation directory for lwip headers ")
set(INSTALL_POSIX_INCLUDE_DIR include/ebbrt-socket/lwip/include/posix CACHE PATH
  "Installation directory for lwip posix headers ")
set(INSTALL_GNU_INCLUDE_DIR include/ebbrt-socket/gnu/include CACHE PATH
  "Installation directory for gnu headers ")

# Install targets
install(TARGETS ebbrt-socket
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  )
install( FILES 
  ${PROJECT_SOURCE_DIR}/SocketManager.h
  ${PROJECT_SOURCE_DIR}/Vfs.h
  DESTINATION include/ebbrt-socket
  )
install( DIRECTORY ${PROJECT_SOURCE_DIR}/gnu DESTINATION
  include/ebbrt-socket FILES_MATCHING PATTERN "*.h")
install( DIRECTORY ${PROJECT_SOURCE_DIR}/lwip DESTINATION
  include/ebbrt-socket FILES_MATCHING PATTERN "*.h")
# Package config file
configure_package_config_file(EbbRTSocketConfig.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/EbbRTSocketConfig.cmake"
  INSTALL_DESTINATION lib/cmake/EbbRTSocket 
  PATH_VARS INSTALL_INCLUDE_DIR INSTALL_LWIP_INCLUDE_DIR
  INSTALL_POSIX_INCLUDE_DIR  INSTALL_GNU_INCLUDE_DIR INSTALL_LIB_DIR
)
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/EbbRTSocketConfig.cmake"
  DESTINATION "lib/cmake/EbbRTSocket" COMPONENT dev)
