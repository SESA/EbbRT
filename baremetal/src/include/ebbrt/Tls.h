//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_TLS_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_TLS_H_

#include <cstring>

namespace ebbrt {
namespace tls {
void Init();
void SmpInit();
void ApInit(size_t size);
}
}

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_TLS_H_
