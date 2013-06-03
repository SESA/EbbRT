#include "ebb/SharedRoot.hpp"
#include "ebb/Console/Console.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "lrt/assert.hpp"

#ifdef __linux__
#include <iostream>
#endif
ebbrt::EbbRoot*
ebbrt::Console::ConstructRoot()
{
  return new SharedRoot<Console>;
}

void
ebbrt::Console::Write(const char* str,
                      std::function<void()> cb)
{
#ifdef __linux__
  std::cout << str;
  if (cb) {
    cb();
  }
#elif __ebbrt__
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
#endif
}

void
ebbrt::Console::HandleMessage(const uint8_t* msg,
                              size_t len)
{
#ifdef __linux__
  std::cout << msg;
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}
