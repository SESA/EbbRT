//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
#define HOSTED_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_

#include <boost/coroutine/coroutine.hpp>

#include <ebbrt/CacheAligned.h>
#include <ebbrt/MoveLambda.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {
class EventManager;

const constexpr auto event_manager = EbbRef<EventManager>(kEventManagerId);

class EventManager : public StaticSharedEbb<EventManager>, public CacheAligned {
 public:
  class EventContext {
   private:
    boost::coroutines::coroutine<void()> coro;
    boost::coroutines::coroutine<void()>::caller_type* caller;

    friend EventManager;
  };

  void Spawn(ebbrt::MovableFunction<void()> func);
  void ActivateContext(EventContext&& context);
  void SaveContext(EventContext& context);

  template <typename F> class HandlerWrapper {
   public:
    explicit HandlerWrapper(const F& f) : f_(f) {}
    explicit HandlerWrapper(F&& f) : f_(std::move(f)) {}
    template <typename... Args> void operator()(Args&&... args) {
      event_manager->active_event_context_.coro =
          boost::coroutines::coroutine<void()>(std::bind(
              [this](boost::coroutines::coroutine<void()>::caller_type& ca,
                     Args&&... args) {
                event_manager->active_event_context_.caller = &ca;
                f_(std::forward<Args>(args)...);
              },
              std::placeholders::_1, std::forward<Args>(args)...));
    }

   private:
    F f_;
  };

  template <typename F> static HandlerWrapper<F> WrapHandler(F&& f) {
    return HandlerWrapper<F>(std::forward<F>(f));
  }

 private:
  EventContext active_event_context_;
};
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
