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
#include <ebbrt/EventContext.h>

namespace ebbrt {
class EventManager;
using ebbrt::EventContext;

const constexpr auto event_manager = EbbRef<EventManager>(kEventManagerId);

class EventManager : public StaticSharedEbb<EventManager>, public CacheAligned {
 public:
  typedef ebbrt::EventContext EventContext;
  void Spawn(ebbrt::MovableFunction<void()> func, ebbrt::Context* ctxt,
             bool force_async = false);
  void Spawn(ebbrt::MovableFunction<void()> func, bool force_async = false) {
    Spawn(std::move(func), active_context, force_async);
  }
  void SpawnRemote(ebbrt::MovableFunction<void()> func, ebbrt::Context* ctxt) {
    Spawn(std::move(func), ctxt);
  }
  void ActivateContext(EventManager::EventContext&& context);
  void SaveContext(EventManager::EventContext& context);

  template <typename F> class HandlerWrapper {
   public:
    explicit HandlerWrapper(const F& f) : f_(f) {}
    explicit HandlerWrapper(F&& f) : f_(std::move(f)) {}
    template <typename... Args> void operator()(Args&&... args) {
      ebbrt::Context* ctxt = active_context;
      ctxt->active_event_context_.coro = boost::coroutines::coroutine<void()>(
          std::bind([this, ctxt](
                        boost::coroutines::coroutine<void()>::caller_type& ca,
                        Args&&... args) {
                      ctxt->active_event_context_.caller = &ca;
                      ca();
                      try {
                        f_(std::forward<Args>(args)...);
                      }
                      catch (std::exception& e) {
                        fprintf(stderr, "Unhandled exception caught: %s\n",
                                e.what());
                        std::abort();
                      }
                      catch (...) {
                        fprintf(stderr, "Unhandled exception caught \n");
                        std::abort();
                      }
                    },
                    std::placeholders::_1, std::forward<Args>(args)...));
      ctxt->active_event_context_.coro();
    }

   private:
    F f_;
  };

  template <typename F> static HandlerWrapper<F> WrapHandler(F&& f) {
    return HandlerWrapper<F>(std::forward<F>(f));
  }
};
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
