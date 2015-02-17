//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_CONSOLE_H_
#define COMMON_SRC_INCLUDE_EBBRT_CONSOLE_H_

namespace ebbrt {
namespace console {
__attribute__((no_instrument_function)) void Init() noexcept;
__attribute__((no_instrument_function)) void Write(const char* str) noexcept;
__attribute__((no_instrument_function)) int  Write(const char* buf, int len) noexcept;
}
}

#endif  // COMMON_SRC_INCLUDE_EBBRT_CONSOLE_H_
