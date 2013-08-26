/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef EBBRT_EBB_EVENTMANAGER_MOVEFUNCTION_HPP
#define EBBRT_EBB_EVENTMANAGER_MOVEFUNCTION_HPP

#include <memory>
#include <tuple>

template <int...>
struct seq {};

//Construct a sequence to be passed to std::get
// gens<N>::type produces seq<1, 2, 3, ..., N>
template <int N, int... S>
struct gens : gens<N-1, N-1, S...> {};

template <int... S>
struct gens<0, S...>
{
  typedef seq<S...> type;
};

template <typename F, typename... BoundArgs>
class move_lambda {
  F f_;

  std::tuple<typename std::remove_reference<BoundArgs>::type...> args_;
public:
  template <typename... Args>
  move_lambda(F f, Args&&... args)
    : f_(std::move(f)), args_{std::forward<Args>(args)...} {}

  // Default move operations
  move_lambda(move_lambda&&) = default;
  move_lambda& operator=(move_lambda&&) = default;

  // No default constructor
  move_lambda() = delete;

  // No copy operations
  move_lambda(const move_lambda&) = delete;
  move_lambda& operator=(const move_lambda&) = delete;

  template<typename... CallArgs>
  typename std::result_of<F(typename std::remove_reference<BoundArgs>::type...,
                            CallArgs...)>::type
  operator()(CallArgs&&... callargs)
  {
    return call(typename gens<sizeof...(BoundArgs)>::type(),
                std::forward<CallArgs...>(callargs)...);
  }
private:
  template<typename... CallArgs, int... S>
  typename std::result_of<F(typename std::remove_reference<BoundArgs>::type...,
                            CallArgs...)>::type
  call(seq<S...>, CallArgs... callargs)
  {
    return f_(std::move(std::get<S>(args_))...,
              std::forward<CallArgs>(callargs)...);
  }
};

template<typename F, typename... Args>
move_lambda<F, Args...>
move_bind(F&& f, Args&&... args)
{
  return move_lambda<F, Args...>
    (std::forward<F>(f), std::forward<Args>(args)...);
}

template< class ReturnType, class... ParamTypes>
struct move_function_base{
  virtual ReturnType callFunc(ParamTypes... p) = 0;
};

template<class F, class ReturnType, class... ParamTypes>
class move_function_imp : public move_function_base<ReturnType, ParamTypes...> {

  F f_;

public:
  virtual ReturnType callFunc(ParamTypes... p) override {
    return f_(std::forward<ParamTypes>(p)...);
  }
  explicit move_function_imp(F f) : f_(std::forward<F>(f)) {}

  move_function_imp() = delete;
  move_function_imp(const move_function_imp&) = delete;
  move_function_imp& operator=(const move_function_imp&) = delete;
};


template<class FuncType>
struct move_function{};

template<class ReturnType, class... ParamTypes>
struct move_function<ReturnType(ParamTypes...)>{
  std::unique_ptr<move_function_base<ReturnType,ParamTypes...>> ptr_;

  move_function() = default;
  template<class F>
  move_function(F&& f) :
    ptr_(new move_function_imp<F, ReturnType,ParamTypes...>
         (std::forward<F>(f))){}
  move_function(move_function&& other) = default;
  move_function& operator=(move_function&& other) = default;

  template<class... Args>
  ReturnType
  operator()(Args&& ...args)
  {
    return ptr_->callFunc(std::forward<Args>(args)...);
  }

  explicit operator bool() const
  {
    return static_cast<bool>(ptr_);
  }

  move_function(const move_function&) = delete;
  move_function& operator=(const move_function&) = delete;
};

#endif
