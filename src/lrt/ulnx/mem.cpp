#include <stdio.h>
#include <cstdint>

#include "lrt/console.hpp"


void
ebbrt::lrt::console::init()
{
  return;
}

void
ebbrt::lrt::console::write(char c)
{
  /* kludge */
  printf("%c",c);
}

int
ebbrt::lrt::console::write(const char *str, int len)
{
  /* kludge */
  printf("%s", str);
  return len;
}

void
ebbrt::lrt::console::write(const char *str)
{
  /* kludge */
  printf("%s", str);
}
