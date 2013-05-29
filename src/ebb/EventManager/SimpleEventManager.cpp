#include "ebb/SharedRoot.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#include "lrt/assert.hpp"

ebbrt::EbbRoot*
ebbrt::SimpleEventManager::ConstructRoot()
{
  return new SharedRoot<SimpleEventManager>();
}

ebbrt::SimpleEventManager::SimpleEventManager() : next_{32}
{
}

uint8_t
ebbrt::SimpleEventManager::AllocateInterrupt(std::function<void()> func)
{
  lock_.Lock();
  uint8_t ret = next_++;
  map_.insert(std::make_pair(ret, std::move(func)));
  lock_.Unlock();
  return ret;
}

void
ebbrt::SimpleEventManager::HandleInterrupt(uint8_t interrupt)
{
  lock_.Lock();
  LRT_ASSERT(map_.find(interrupt) != map_.end());
  const auto& f = map_[interrupt];
  lock_.Unlock();
  if (f) {
    f();
  }
}
