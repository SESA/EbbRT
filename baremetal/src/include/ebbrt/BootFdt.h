//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_BOOTFDT_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_BOOTFDT_H_

#include <ebbrt/Fdt.h>
#include <ebbrt/Multiboot.h>

namespace ebbrt {
namespace boot_fdt {
void Init(multiboot::Information* mbi);
FdtReader Get();
}  // namespace boot_fdt
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_BOOTFDT_H_
