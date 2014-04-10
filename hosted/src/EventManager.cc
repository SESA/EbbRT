//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/EventManager.h>

#include <ebbrt/Context.h>

void ebbrt::EventManager::Spawn(MovableFunction<void()> func) {
  // movable function cannot be copied so we put it on the heap
  auto f = new MovableFunction<void()>(std::move(func));
  // async dispatch
  active_context->io_service_.post([f, this]() {
    // create and run a new coroutine to run the event on
    active_event_context_.coro = boost::coroutines::coroutine<void()>([f, this](
        boost::coroutines::coroutine<void()>::caller_type& ca) {
      ca();
      // store the caller for later if we need to save the context
      active_event_context_.caller = &ca;
      (*f)();
      delete f;
    });
    active_event_context_.coro();
  });
}

void ebbrt::EventManager::ActivateContext(EventContext&& context) {
  auto c = new EventContext(std::move(context));
  active_context->io_service_.post([c, this]() {
    active_event_context_ = std::move(*c);
    active_event_context_.coro();
    delete c;
  });
}

void ebbrt::EventManager::SaveContext(EventContext& context) {
  context = std::move(active_event_context_);
  (*context.caller)();
}
