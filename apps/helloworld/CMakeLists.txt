project("helloworld-ebbrt" CXX)
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g3")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")

set(HOSTED_SOURCES 
      src/hosted/helloworld.cc 
      src/hosted/Printer.cc)

set(BAREMETAL_SOURCES 
      src/baremetal/helloworld.cc 
      src/baremetal/Printer.cc)

# Baremetal  ========================================================
# Build for baremetal by -DCMAKE_TOOLCHAIN_FILE to point to your sysroot
# directory. e.g., cmake -DCMAKE_TOOLCHAIN_FILE=~/EbbRT/toolchain/sysroot 

if(CMAKE_TOOLCHAIN_FILE)
  message(STATUS "Sourcing EbbRT toolchain")
  include(${CMAKE_TOOLCHAIN_FILE})
endif()

if( ${CMAKE_SYSTEM_NAME} STREQUAL "EbbRT")

  message(STATUS "Building baremetal image...")
  add_executable(helloworld.elf ${BAREMETAL_SOURCES})
  add_custom_command(TARGET helloworld.elf POST_BUILD 
    COMMAND objcopy -O elf32-i386 helloworld.elf helloworld.elf32 )

# Hosted  ===========================================================
# To build from the hosted with cmake need to specify the locations of
# libEbbRT and the EbbRT headers. By default, these values are stored in a
# cmake configuration file (EbbRTConfig.cmake) that is created when you
# build/install EbbRT hosted.
#
# Specify the directory location of EbbRTConfig.cmake by setting the
# $EBBRT_CONFIGDIR environment variable. Alternately, you can set EBBRT_LIBDIR
# to specify the installed location of EbbRT hosted, wherein EbbRTConfig.cmake
# is installed to: ./lib/cmake/EbbRT/ 

elseif( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  message(STATUS "Building Linux executable...")
  # optional: pull cmake modules from the EbbRT source tree 
  # TODO(jmcadden): remove source-dir requirement by installing these modules
  # to the system
  if(DEFINED ENV{EBBRT_SRCDIR})
    list(APPEND CMAKE_MODULE_PATH 
      "$ENV{EBBRT_SRCDIR}/hosted/cmake/Modules")
  endif()

  # optional: specify location of cmake configuration (EbbRTConfig.cmake)
  if(DEFINED ENV{EbbRT_DIR})
    message( STATUS "Location for EbbRTConfig.cmake found ($EbbRT_DIR)")
  else()
    message( WARNING "No location specified for EbbRTConfig.cmake ($EbbRT_DIR)")
  endif()

  find_package(EbbRT REQUIRED)
  find_package(Boost 1.53.0 REQUIRED COMPONENTS 
    filesystem system coroutine context )
  find_package(Capnp REQUIRED)
  find_package(Fdt REQUIRED)
  find_package(TBB REQUIRED)
  
  list(APPEND CMAKE_CXX_FLAGS " -std=gnu++14 -Wall -Werror ")

  include_directories(${EBBRT_INCLUDE_DIRS})
  add_executable(helloworld ${HOSTED_SOURCES})
  target_link_libraries(helloworld -Wl,--whole-archive ${EBBRT_LIBRARIES}
    -Wl,--no-whole-archive -lpthread
    ${CAPNP_LIBRARIES_LITE} ${LIBFDT_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} ${TBB_LIBRARIES} 
    )

else()
  message(WARNING "You have not specified the location of the baremetal
  toolchain (-DCMAKE_TOOLCHAIN_FILE=) or the hosted configuration
  (EbbRT_DIR=). Build can not proceed.")
  message(FATAL_ERROR "System name unsupported: ${CMAKE_SYSTEM_NAME}")
endif()
