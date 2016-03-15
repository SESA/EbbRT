//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_RCU_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_RCU_H_

#include <ebbrt/EventManager.h>
#include <ebbrt/Future.h>

namespace ebbrt {
// non void return implementation
template <typename F, typename... Args>
typename std::enable_if<
    !std::is_void<typename std::result_of<F()>::type>::value,
    Future<typename Flatten<typename std::result_of<F(Args...)>::type>::type>>::
    type
    CallRcuHelper(F&& f, Args&&... args) {
  typedef typename std::result_of<F(Args...)>::type result_type;
  typedef decltype(std::bind(std::forward<F>(f), std::forward<Args>(args)...))
      bound_fn_type;
  auto p = Promise<result_type>();
  auto ret = p.GetFuture();
  auto bound_f = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
  event_manager->DoRcu(
      MoveBind([](Promise<result_type> prom, bound_fn_type fn) {
                 try {
                   prom.SetValue(fn());
                 }
                 catch (...) {
                   prom.SetException(std::current_exception());
                 }
               },
               std::move(p), std::move(bound_f)));
  return flatten(std::move(ret));
}

// non void return implementation
template <typename F, typename... Args>
typename std::enable_if<
    std::is_void<typename std::result_of<F()>::type>::value,
    Future<typename Flatten<typename std::result_of<F(Args...)>::type>::type>>::
    type
    CallRcuHelper(F&& f, Args&&... args) {
  auto p = Promise<void>();
  auto ret = p.GetFuture();
  auto bound_f = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
  event_manager->DoRcu([prom = std::move(p),
                        fn = std::move(bound_f)]() mutable {
                                  try {
                                    fn();
                                    prom.SetValue();
                                  }
                                  catch (...) {
                                    prom.SetException(std::current_exception());
                                  }
                                });
  return flatten(std::move(ret));
}

template <typename F, typename... Args>
Future<typename Flatten<typename std::result_of<F(Args...)>::type>::type>
CallRcu(F&& f, Args&&... args) {
  return CallRcuHelper(std::forward<F>(f), std::forward<Args>(args)...);
}

}  // namespace ebbrt
#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_RCU_H_
