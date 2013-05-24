#include "ebb/SharedRoot.hpp"
#include "ebb/Console/Console.hpp"

char ebbrt::console_id_resv __attribute__ ((section ("static_ebb_ids")));

ebbrt::EbbRoot*
ebbrt::Console::ConstructRoot()
{
  return new SharedRoot<Console>;
}

#include "lrt/assert.hpp"

void
ebbrt::Console::Write(const char* str)
{
  LRT_ASSERT(0);
}
