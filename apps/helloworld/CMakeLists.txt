cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
# This skips compiler verification which will fail because we haven't finished
# installing the necessary libraries
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
project("helloworld-ebbrt" C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g3")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wall -Werror")

set(HOSTED_SOURCES
      src/hosted/helloworld.cc
      src/hosted/Printer.cc)

set(BAREMETAL_SOURCES
      src/baremetal/helloworld.cc
      src/baremetal/Printer.cc)

# Baremetal  ========================================================

if( ${CMAKE_SYSTEM_NAME} STREQUAL "EbbRT")

  add_executable(helloworld.elf ${BAREMETAL_SOURCES})
  add_custom_command(TARGET helloworld.elf POST_BUILD
    COMMAND objcopy -O elf32-i386 helloworld.elf helloworld.elf32 )

# Hosted  ===========================================================
elseif( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )

  # TODO(jmcadden): remove source-dir requirement
  if(DEFINED ENV{EBBRT_SRCDIR})
    list(APPEND CMAKE_MODULE_PATH
      "$ENV{EBBRT_SRCDIR}/hosted/cmake/Modules")
  endif()



  find_package(EbbRT REQUIRED)
  find_package(Boost 1.85.0 REQUIRED COMPONENTS 
    filesystem system coroutine context )
  find_package(CapnProto REQUIRED)
  find_package(TBB REQUIRED)
  find_package(Threads REQUIRED)
  include_directories(${Boost_INCLUDE_DIRS})
  include_directories(${CAPNP_INCLUDE_DIRS})
  include_directories(${TBB_INCLUDE_DIRS})

  include_directories(${EBBRT_INCLUDE_DIRS})
  add_executable(helloworld ${HOSTED_SOURCES})
  target_link_libraries(helloworld ${EBBRT_LIBRARIES}
    ${CAPNP_LIBRARIES_LITE} ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES} ${TBB_LIBRARIES}
  )
else()
  message(FATAL_ERROR "System name unsupported: ${CMAKE_SYSTEM_NAME}")
endif()
