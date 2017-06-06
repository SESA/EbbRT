cmake_minimum_required(VERSION 2.6)
project(EbbRT-libs CXX ASM)

# EbbRT Library to build by default
set(subdirs
  cmdline
  filesystem
  socket
)
foreach(subdir ${subdirs})
  add_subdirectory(${subdir})
endforeach()
