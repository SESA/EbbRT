# EbbRT hosted platform-specific configuration
add_compile_options(-std=gnu++14 -Wall -Werror)

find_package(Boost 1.53.0 REQUIRED COMPONENTS 
  filesystem system coroutine context )
find_package(Capnp REQUIRED)
find_package(TBB REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CAPNP_INCLUDE_DIRS})
include_directories(${TBB_INCLUDE_DIRS})
