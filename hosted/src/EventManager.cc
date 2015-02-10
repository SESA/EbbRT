//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/EventManager.h>

#include <ebbrt/Context.h>

void ebbrt::EventManager::Spawn(MovableFunction<void()> func, ebbrt::Context *ctxt) {
  // movable function cannot be copied so we put it on the heap
  auto f = new MovableFunction<void()>(std::move(func));
  // async dispatch
#if 0
  if (ctxt == active_context)  printf("Local: "); else printf("Remote: ");
  printf("%zd->%zd\n", active_context->index_, ctxt->index_);
#endif
 
  ctxt->io_service_.post([f, this, ctxt]() {
    // create and run a new coroutine to run the event on
 ctxt->active_event_context_.coro = boost::coroutines::coroutine<void()>([f, this,ctxt](
        boost::coroutines::coroutine<void()>::caller_type& ca) {
      ca();
      // store the caller for later if we need to save the context
      ctxt->active_event_context_.caller = &ca;
      (*f)();
      delete f;
    });
    ctxt->active_event_context_.coro();
  });
}

void ebbrt::EventManager::ActivateContext(EventManager::EventContext&& context) {
  auto c = new EventManager::EventContext(std::move(context));
  ebbrt::Context *ctxt = active_context;
  ctxt->io_service_.post([c, this, ctxt]() {
    ctxt->active_event_context_ = std::move(*c);
    ctxt->active_event_context_.coro();
    delete c;
  });
}

void ebbrt::EventManager::SaveContext(EventManager::EventContext& context) {
  context = std::move(active_context->active_event_context_);
  (*context.caller)();
}
