#ifndef EBBRT_EBB_EVENTMANAGER_SIMPLEEVENTMANAGER_HPP
#define EBBRT_EBB_EVENTMANAGER_SIMPLEEVENTMANAGER_HPP

#include <unordered_map>

#include "ebb/EventManager/EventManager.hpp"
#include "sync/spinlock.hpp"

namespace ebbrt {
  class SimpleEventManager : public EventManager {
  public:
    static EbbRoot* ConstructRoot();

    SimpleEventManager();
    uint8_t AllocateInterrupt(std::function<void()> func) override;
  private:
    void HandleInterrupt(uint8_t interrupt) override;

    std::unordered_map<uint8_t, std::function<void()> > map_;
    uint8_t next_;
    Spinlock lock_;
  };
}

#endif
