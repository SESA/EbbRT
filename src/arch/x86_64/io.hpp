#ifndef EBBRT_ARCH_IO_HPP
#error "Don't include this file directly"
#endif

inline void
ebbrt::outb(int8_t val, int16_t port)
{
  asm volatile ("outb %b[val], %w[port]"
                : //no output
                : [val] "a" (val),
                [port] "Nd" (port));
}

inline int8_t
ebbrt::inb(int16_t port)
{
  int8_t val;
  asm volatile ("inb %w[port], %b[val]"
                : [val] "=a" (val)
                : [port] "Nd" (port));
  return val;
}
