#pragma once

#include "ebb/EventManager/Future.hpp"

void activate_context();

void deactivate_context();

extern std::unique_ptr<ebbrt::Context> context;

template <typename T>
inline T wait_for_future(ebbrt::Future<T> *fut) {
  T ret;
  fut->Then(ebbrt::launch::async, [&ret](ebbrt::Future<T> complete) {
      ret = std::move(complete.Get());
      context->Break();
    });
  context->Loop(-1);
  return ret;
}

template <>
inline void wait_for_future(ebbrt::Future<void> *fut) {
  fut->Then(ebbrt::launch::async, [](ebbrt::Future<void> complete) {
      complete.Get();
      context->Break();
    });
  context->Loop(-1);
}
