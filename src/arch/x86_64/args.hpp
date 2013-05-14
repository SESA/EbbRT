#ifndef EBBRT_ARCH_ARGS_HPP
#error "Don't include this file directly"
#endif

#include <cstdint>

namespace ebbrt {
  class Args {
  public:
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t rax;
    uint64_t fx[64];
    uint64_t ret;
    uint64_t stack_args[0];
  };
}
