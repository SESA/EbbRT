//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_LWIPCC_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_LWIPCC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef BYTE_ORDER
#if __BYTE_ORDER == __ORDER_BIG_ENDIAN__
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif
#endif

typedef uint8_t u8_t;
typedef int8_t s8_t;
typedef uint16_t u16_t;
typedef int16_t s16_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;

typedef uintptr_t mem_ptr_t;

#define LWIP_ERR_T int

#define U16_F PRIu16
#define S16_F PRId16
#define X16_F PRIx16
#define U32_F PRIu32
#define S32_F PRId32
#define X32_F PRIx32

#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

extern void lwip_printf(const char* fmt, ...);

#define LWIP_PLATFORM_DIAG(x)                                                  \
  do {                                                                         \
    lwip_printf x;                                                             \
  } while (0)

extern void lwip_assert(const char* fmt, ...);

#define LWIP_PLATFORM_ASSERT(x)                                                \
  do {                                                                         \
    lwip_assert(x);                                                            \
  } while (1)

extern uint32_t lwip_rand();

#define LWIP_RAND() lwip_rand()

#ifdef __cplusplus
}
#endif

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_LWIPCC_H_
