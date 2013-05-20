#ifndef EBBRT_LRT_BOOT_HPP
#define EBBRT_LRT_BOOT_HPP

#ifdef LRT_ULNX
#include <src/lrt/ulnx/boot.hpp>
#elif LRT_BARE
#include <src/lrt/bare/boot.hpp>
#endif

#endif
