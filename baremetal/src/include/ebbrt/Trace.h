//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_TRACE_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_TRACE_H_

namespace ebbrt {
namespace trace {
__attribute__((no_instrument_function)) void Init() noexcept;
__attribute__((no_instrument_function)) void Dump() noexcept;
__attribute__((no_instrument_function)) void enable_trace() noexcept;
__attribute__((no_instrument_function)) void disable_trace() noexcept;
}
}
#endif 
