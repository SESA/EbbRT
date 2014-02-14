//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Context.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/NodeAllocator.h>
#include <ebbrt/Runtime.h>

ebbrt::Runtime::Runtime() : initialized_(false), indices_(0) {}

void ebbrt::Runtime::Initialize() {
  if (active_context == nullptr) {
    throw std::runtime_error(
        "Runtime cannot initialize without an active context");
  }
  if (active_context->index_ == 0) {
    DoInitialization();

    std::lock_guard<std::mutex> lock(init_lock_);
    initialized_ = true;
    init_cv_.notify_all();
  } else {
    std::unique_lock<std::mutex> lock(init_lock_);
    if (!initialized_) {
      init_cv_.wait(lock, [&] { return initialized_; });
    }
  }
}

void ebbrt::Runtime::DoInitialization() {
  EventManager::Init();
  LocalIdMap::Init();
  EbbAllocator::Init();
  Messenger::Init();
  NodeAllocator::Init();
}
