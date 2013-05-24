#include "ebb/SharedRoot.hpp"
#include "ebb/Console/Console.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

char ebbrt::console_id_resv __attribute__ ((section ("static_ebb_ids")));

ebbrt::EbbRoot*
ebbrt::Console::ConstructRoot()
{
  return new SharedRoot<Console>;
}

#include "lrt/assert.hpp"

void
ebbrt::Console::Write(const char* str,
                      const std::function<void(const char*)>& cb)
{
  BufferList list = BufferList(1, std::make_pair(str, strlen(str) + 1));
  LRT_ASSERT(!cb);
  message_manager->Send(0, console, std::move(list));
}
