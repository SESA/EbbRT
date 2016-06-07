set(CMAKE_SYSTEM_NAME EbbRT)

if (NOT DEFINED ENV{EBBRT_SYSROOT})
  message(FATAL_ERROR "EBBRT_SYSROOT environment variable not set")
endif()

if (NOT IS_DIRECTORY $ENV{EBBRT_SYSROOT})
  message(FATAL_ERROR "EBBRT_SYSROOT is not a directory")
endif()

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH  $ENV{EBBRT_SYSROOT})

# specify the cross compiler
SET(CMAKE_C_COMPILER   ${CMAKE_FIND_ROOT_PATH}/usr/bin/x86_64-pc-ebbrt-gcc)
SET(CMAKE_CXX_COMPILER ${CMAKE_FIND_ROOT_PATH}/usr/bin/x86_64-pc-ebbrt-g++)
SET(CMAKE_ASM_COMPILER ${CMAKE_FIND_ROOT_PATH}/usr/bin/x86_64-pc-ebbrt-g++)
SET(CMAKE_AR ${CMAKE_FIND_ROOT_PATH}/usr/bin/x86_64-pc-ebbrt-gcc-ar CACHE FILEPATH "Archiver")#
SET(CMAKE_RANLIB ${CMAKE_FIND_ROOT_PATH}/usr/bin/x86_64-pc-ebbrt-gcc-ranlib CACHE FILEPATH "Archiver")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=gnu++14" )

SET(CMAKE_SYSTEM_PREFIX_PATH ${CMAKE_FIND_ROOT_PATH}/usr)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
