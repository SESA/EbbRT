#include "lwip/sockets.h"
#include "posix/arpa/inet_aton.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t htonl(uint32_t hostlong);

uint16_t htons(uint16_t hostshort);

uint32_t ntohl(uint32_t netlong);

uint16_t ntohs(uint16_t netshort);

#ifdef __cplusplus
}
#endif
