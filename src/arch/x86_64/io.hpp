#ifndef EBBRT_ARCH_IO_HPP
#error "Don't include this file directly"
#endif

inline void
ebbrt::out8(uint8_t val, uint16_t port)
{
  asm volatile ("outb %b[val], %w[port]"
                : //no output
                : [val] "a" (val),
                [port] "Nd" (port));
}

inline uint8_t
ebbrt::in8(uint16_t port)
{
  uint8_t val;
  asm volatile ("inb %w[port], %b[val]"
                : [val] "=a" (val)
                : [port] "Nd" (port));
  return val;
}

inline void
ebbrt::out32(uint32_t val, uint16_t port)
{
  asm volatile ("outl %[val], %w[port]"
                : //no output
                : [val] "a" (val),
                [port] "Nd" (port));
}

inline uint32_t
ebbrt::in32(uint16_t port)
{
  uint32_t val;
  asm volatile ("inl %w[port], %[val]"
                : [val] "=a" (val)
                : [port] "Nd" (port));
  return val;
}
