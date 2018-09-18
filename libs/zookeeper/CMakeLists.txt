project("ebbrt-zookeeper")
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
include(CMakePackageConfigHelpers)
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g3")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wno-unused-variable -Wno-unused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable -Wno-unused-function")

# Platform-specific package dependencies
if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  find_package(EbbRT REQUIRED)
endif()
if( ${CMAKE_SYSTEM_NAME} STREQUAL "EbbRT")
  find_library(EbbRT REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
  find_package(EbbRTSocket REQUIRED)
  include_directories(${EBBRT-SOCKET_INCLUDE_DIRS})
endif()

# Library targets
set(LIB_INCLUDES
  zookeeper/include 
)
set(LIB_SOURCES 
  zookeeper/src/hashtable/hashtable.c
  zookeeper/src/hashtable/hashtable_itr.c
  zookeeper/src/recordio.c
  zookeeper/src/st_adaptor.c
  zookeeper/src/winport.c
  zookeeper/src/zk_hashtable.c
  zookeeper/src/zk_log.c
  zookeeper/src/zookeeper.c
  zookeeper/src/zookeeper.jute.c
  ZooKeeper.cc 
)
include_directories(${LIB_INCLUDES})
include_directories(${EBBRT_INCLUDE_DIRS})
add_library(ebbrt-zookeeper ${LIB_SOURCES} )

# Installation directories 
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH
  "Installation directory for header files")
set(INSTALL_ZOOKEEPER_INCLUDE_DIR include/ebbrt-zookeeper/zookeeper/include CACHE PATH
  "Installation directory for zookeeper client headers ")
set(INSTALL_CMAKE_DIR lib/cmake/EbbRTZooKeeper CACHE PATH
  "Installation directory for CMake files")

# Install targets
install(TARGETS ebbrt-zookeeper
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  )
install( FILES 
  ${PROJECT_SOURCE_DIR}/ZooKeeper.h
  ${PROJECT_SOURCE_DIR}/ZKGlobalIdMap.h
  DESTINATION include/ebbrt-zookeeper
  )
install( DIRECTORY ${PROJECT_SOURCE_DIR}/zookeeper DESTINATION
  include/ebbrt-zookeeper FILES_MATCHING PATTERN "*.h")

# Package config file
configure_package_config_file(EbbRTZooKeeperConfig.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/EbbRTZooKeeperConfig.cmake"
  INSTALL_DESTINATION ${INSTALL_CMAKE_DIR} 
  PATH_VARS INSTALL_INCLUDE_DIR INSTALL_ZOOKEEPER_INCLUDE_DIR INSTALL_LIB_DIR
)
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/EbbRTZooKeeperConfig.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)
