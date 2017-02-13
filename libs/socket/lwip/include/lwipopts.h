#include "errno.h"
#define LWIP_NETCONN     1
#define NO_SYS           1
#define MEM_LIBC_MALLOC  1
#define MEMP_MEM_MALLOC  1
#define LWIP_DNS         1
#define LWIP_TIMEVAL_PRIVATE 0 /* timeval available in time.h */
#define SYS_ARCH_TIMEOUT 0
#define LWIP_PREFIX_BYTEORDER_FUNCS 0



