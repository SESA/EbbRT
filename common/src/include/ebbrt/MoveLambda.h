//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_MOVELAMBDA_H_
#define COMMON_SRC_INCLUDE_EBBRT_MOVELAMBDA_H_

#include <memory>

namespace ebbrt {
template <typename ReturnType, typename... ParamTypes>
class MovableFunctionBase {
 public:
  virtual ReturnType CallFunc(ParamTypes... p) = 0;
};

template <typename F, typename ReturnType, typename... ParamTypes>
class MovableFunctionImp
    : public MovableFunctionBase<ReturnType, ParamTypes...> {
 public:
  typedef typename std::decay<F>::type f_type;
  MovableFunctionImp() = delete;
  MovableFunctionImp(const MovableFunctionImp&) = delete;
  explicit MovableFunctionImp(const f_type& f) : f_(f) {}
  explicit MovableFunctionImp(f_type&& f) : f_(std::move(f)) {}

  MovableFunctionImp& operator=(const MovableFunctionImp&) = delete;
  ReturnType CallFunc(ParamTypes... p) override {
    return f_(std::forward<ParamTypes>(p)...);
  }

 private:
  f_type f_;
};

template <typename F, typename... ParamTypes>
class MovableFunctionImp<F, void, ParamTypes...> : public MovableFunctionBase<
                                                       void, ParamTypes...> {
 public:
  typedef typename std::decay<F>::type f_type;
  MovableFunctionImp() = delete;
  MovableFunctionImp(const MovableFunctionImp&) = delete;
  explicit MovableFunctionImp(const f_type& f) : f_(f) {}
  explicit MovableFunctionImp(f_type&& f) : f_(std::move(f)) {}

  MovableFunctionImp& operator=(const MovableFunctionImp&) = delete;
  void CallFunc(ParamTypes... p) override {
    f_(std::forward<ParamTypes>(p)...);
  }

 private:
  f_type f_;
};

template <typename FuncType> class MovableFunction {};

template <typename ReturnType, typename... ParamTypes>
class MovableFunction<ReturnType(ParamTypes...)> {
 public:
  MovableFunction() = default;
  template <typename F>
  MovableFunction(F&& f)
      : ptr_(new MovableFunctionImp<F, ReturnType, ParamTypes...>(
            std::forward<F>(f))) {}
  MovableFunction(const MovableFunction&) = delete;
  MovableFunction(MovableFunction&& other) = default;

  MovableFunction& operator=(const MovableFunction&) = delete;
  MovableFunction& operator=(MovableFunction&& other) = default;
  template <typename... Args> auto operator()(Args&&... args) -> ReturnType {
    return ptr_->CallFunc(std::forward<Args>(args)...);
  }
  explicit operator bool() const { return static_cast<bool>(ptr_); }

 private:
  std::unique_ptr<MovableFunctionBase<ReturnType, ParamTypes...>> ptr_;
};

template <typename... ParamTypes> class MovableFunction<void(ParamTypes...)> {
 public:
  MovableFunction() = default;
  MovableFunction(std::nullptr_t) : ptr_() {}
  template <typename F>
  MovableFunction(
      F&& f,
      // This parameter ensures that this overload can't be used with a
      // MovableFunction (e.g. copy or move)
      typename std::enable_if<
          !std::is_same<typename std::remove_reference<F>::type,
                        MovableFunction<void(ParamTypes...)>>::value>::type* =
          0)
      : ptr_(new MovableFunctionImp<F, void, ParamTypes...>(
            std::forward<F>(f))) {}
  MovableFunction(const MovableFunction&) = delete;
  MovableFunction(MovableFunction&& other) = default;

  MovableFunction& operator=(const MovableFunction&) = delete;
  MovableFunction& operator=(MovableFunction&& other) = default;
  template <typename... Args> void operator()(Args&&... args) {
    ptr_->CallFunc(std::forward<Args>(args)...);
  }
  explicit operator bool() const { return static_cast<bool>(ptr_); }

 private:
  std::unique_ptr<MovableFunctionBase<void, ParamTypes...>> ptr_;
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_MOVELAMBDA_H_
