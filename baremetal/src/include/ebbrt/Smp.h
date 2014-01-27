//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_SMP_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_SMP_H_

#define SMP_START_ADDRESS (0x4000)

#ifndef ASSEMBLY
namespace ebbrt {
namespace smp {
  void Init();
  extern "C" __attribute__((noreturn)) void SmpMain();
}
}
#endif

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_SMP_H_
