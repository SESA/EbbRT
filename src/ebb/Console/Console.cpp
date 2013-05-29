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
                      std::function<void()> cb)
{
  BufferList list = BufferList(1, std::make_pair(str, strlen(str) + 1));
  LRT_ASSERT(!cb);
  NetworkId id;
  id.mac_addr[0] = 0xff;
  id.mac_addr[1] = 0xff;
  id.mac_addr[2] = 0xff;
  id.mac_addr[3] = 0xff;
  id.mac_addr[4] = 0xff;
  id.mac_addr[5] = 0xff;
  message_manager->Send(id, console, std::move(list));
}
