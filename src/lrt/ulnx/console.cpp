#include <cstdio>
#include <cstdint>

#include "lrt/console.hpp"


void
ebbrt::lrt::console::init()
{
  /* no init needed */
  return;
}

void
ebbrt::lrt::console::write(char c)
{
  std::printf("%c", c);
}

int
ebbrt::lrt::console::write(const char *str, int len)
{
  std::printf("%s", str);
  return len;
}

void
ebbrt::lrt::console::write(const char *str)
{
  std::printf("%s", str);
}
