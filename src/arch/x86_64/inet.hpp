#ifndef EBBRT_ARCH_INET_HPP
#error "Don't include this file directly"
#endif

inline uint16_t
ebbrt::htons(uint16_t hostshort)
{
  return hostshort << 8 | hostshort >> 8;
}
