cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project("{{ title }}-ebbrt" C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g3")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wall -Werror")
set(BAREMETAL_SOURCES src/{{ title }}.cc)

if( ${CMAKE_SYSTEM_NAME} STREQUAL "EbbRT")
  add_executable({{ title }}.elf ${BAREMETAL_SOURCES})
  add_custom_command(TARGET {{ title }}.elf POST_BUILD
    COMMAND objcopy -O elf32-i386 {{ title }}.elf {{ title }}.elf32 )

# Hosted  ===========================================================
elseif( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  message(FATAL_ERROR "System name unsupported: ${CMAKE_SYSTEM_NAME}")
endif()
