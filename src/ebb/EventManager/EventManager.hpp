#ifndef EBBRT_EBB_EVENTMANAGER_EVENTMANAGER_HPP
#define EBBRT_EBB_EVENTMANAGER_EVENTMANAGER_HPP

#include <functional>

#include "ebb/ebb.hpp"
#include "lrt/event.hpp"

namespace ebbrt {
  class EventManager : public EbbRep {
  public:
    virtual uint8_t AllocateInterrupt(std::function<void()> func) = 0;
  private:
    friend void lrt::event::_event_interrupt(uint8_t interrupt);
    virtual void HandleInterrupt(uint8_t interrupt) = 0;
  };
  const EbbRef<EventManager> event_manager =
    EbbRef<EventManager>(lrt::trans::find_static_ebb_id("EventManager"));
}
#endif
