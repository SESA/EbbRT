#ifndef LRT_ASSERT_HPP
#define LRT_ASSERT_HPP

#ifdef NDEBUG
#define LRT_ASSERT(expr)
#else
#include "lrt/console.hpp"
#define STR(x) #x
#define STR2(x) STR(x)
#define LRT_ASSERT(expr)						\
  do {									\
    if (!(expr)) {							\
      ebbrt::lrt::console::write("Assertion failed: " __FILE__ ", line " \
                                 STR2(__LINE__) ": " #expr "\n");	\
      while (1)								\
        ;								\
    }									\
  } while (0)

#endif

#endif
