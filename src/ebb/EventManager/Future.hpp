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
#ifndef EBBRT_EBB_EVENTMANAGER_FUTURE_HPP
#define EBBRT_EBB_EVENTMANAGER_FUTURE_HPP

#include <cassert>

#include <atomic>
#include <forward_list>
#include <mutex>

#include "ebb/EventManager/EventManager.hpp"
#include "src/sync/spinlock.hpp"

namespace ebbrt {
  namespace future_internal {
    template <typename T>
    class Promise;

    template <typename T>
    class Future;

    struct make_future_tag{};

    template<class T, class... Args>
    Future<T> make_future(Args&&... args);

    template <typename T>
    class SharedData {
    public:
      SharedData();

      template<class... Args>
      SharedData(make_future_tag tag, Args&&... args);

      // no copy
      SharedData(const SharedData&) = delete;

      // no copy
      SharedData& operator=(const SharedData&) = delete;

      void Set(const T& val);

      void Set(T&& val);

      void Set(Future<T> fut);

      void SetException(const std::exception_ptr& exception);

      Spinlock mutables_;

      static constexpr unsigned PENDING = 0;
      static constexpr unsigned FULFILLED = 1;
      static constexpr unsigned FAILED = 2;

      std::atomic_uint state_;

      T val_;
      std::exception_ptr exception_;

      std::forward_list<std::function<void(T)> > success_funcs_;
      std::forward_list<std::function<void(std::exception_ptr)> > failure_funcs_;
    private:
    };

    template <>
    class SharedData<void> {
    public:
      SharedData();
      // no copy
      SharedData(const SharedData&) = delete;

      // no copy
      SharedData& operator=(const SharedData&) = delete;

      void Set();

      void Set(Future<void> fut);

      void SetException(const std::exception_ptr& exception);

      Spinlock mutables_;

      static constexpr unsigned PENDING = 0;
      static constexpr unsigned FULFILLED = 1;
      static constexpr unsigned FAILED = 2;

      std::atomic_uint state_;

      std::exception_ptr exception_;

      std::forward_list<std::function<void()> > success_funcs_;
      std::forward_list<std::function<void(std::exception_ptr)> > failure_funcs_;
    };

    template <typename T>
    class Future {
    public:
      Future() = default;

      template <typename F> //FIXME: flatten return
      Future<typename std::result_of<F(T)>::type>
      OnSuccess(F f);

      template <typename F> //FIXME: flatten return
      Future<typename std::result_of<F(std::exception_ptr)>::type>
      OnFailure(F f);

      T Get() const;

      std::exception_ptr GetException() const;

      bool Fulfilled() const;

      bool Failed() const;
    private:
      friend class Promise<T>;
      explicit Future(std::shared_ptr<SharedData<T> >& s) :
        s_{s} {}

      template<typename T1, typename... Args>
      friend Future<T1> make_future(Args&&... args);

      template<class... Args>
      Future(make_future_tag tag, Args&&... args);

      std::shared_ptr<SharedData<T> > s_;
    };

    template<class T, class... Args>
    Future<T> make_future(Args&&... args)
    {
      return Future<T>(make_future_tag(), std::forward<Args>(args)...);
    }

    template <>
    class Future<void> {
    public:
      Future();

      template <typename F> //FIXME: flatten return
      Future<typename std::result_of<F()>::type>
      OnSuccess(F f);

      template <typename F> //FIXME: flatten return
      Future<typename std::result_of<F(std::exception_ptr)>::type>
      OnFailure(F f);

      std::exception_ptr GetException() const;

      bool Fulfilled() const;

      bool Failed() const;
    private:
      friend class Promise<void>;
      explicit Future(std::shared_ptr<SharedData<void> >& s) :
        s_{s} {}

      std::shared_ptr<SharedData<void> > s_;
    };

    // template <typename T>
    // struct Flatten {
    //   typedef T type;
    // };

    // template <typename T>
    // struct Flatten<Future<T> > {
    //   typedef typename Flatten<T>::type type;
    // };

    // template <typename T>
    // Future<typename Flatten<T>::type>
    // flatten(Future<T> t,
    //         typename std::enable_if<std::is_same<T, typename Flatten<T>::type>
    //         ::value>::type* = 0) {
    //   return t;
    // }

    // template <typename T>
    // Future<typename Flatten<T>::type>
    // flatten(Future<Future<T> > fut) {
    //   Promise<T> promise;
    //   fut.OnSuccess([=](Future<T> inner) mutable {
    //       inner.OnSuccess([=](T val) mutable {
    //           promise.Set(std::move(val));
    //         });

    //       inner.OnFailure([=](std::exception_ptr eptr) mutable {
    //           promise.SetException(std::move(eptr));
    //         });
    //     });

    //   fut.OnFailure([=](std::exception_ptr eptr) mutable {
    //       promise.SetException(std::move(eptr));
    //     });

    //   return flatten(promise.GetFuture());
    // }

    template <typename T>
    class Promise {
    public:
      Promise();

      void Set(const T& val);

      void Set(T&& val);

      void Set(Future<T> fut);

      void SetException(const std::exception_ptr& exception);

      Future<T> GetFuture();
    private:
      std::shared_ptr<SharedData<T> > i_;
    };

    template <>
    class Promise<void> {
    public:
      Promise();

      void Set();

      void Set(Future<void> fut);

      void SetException(const std::exception_ptr& exception);

      Future<void> GetFuture();
    private:
      std::shared_ptr<SharedData<void> > i_;
    };

    template <typename F>
    Future<typename std::result_of<F()>::type>
    AsyncHelper(F f,
                typename std::enable_if<!std::is_void<typename std::result_of
                <F()>::type>::value>::type* = 0)
    {
      auto promise = new Promise<typename std::result_of<F()>::type>;
      Future<typename std::result_of<F()>::type> ret = promise->GetFuture();
      event_manager->Async([=]() {
          try {
            promise->Set(f());
          } catch (...) {
            promise->SetException(std::current_exception());
          }
          delete promise;
        });
      return std::move(ret);
    }

    template <typename F>
    Future<void>
    AsyncHelper(F f,
                typename std::enable_if<std::is_void<typename std::result_of
                <F()>::type>::value>::type* = 0)
    {
      auto promise = new Promise<void>;
      auto ret = promise->GetFuture();
      event_manager->Async([=]() {
          try {
            f();
            promise->Set();
          } catch (...) {
            promise->SetException(std::current_exception());
          }
          delete promise;
        });
      return std::move(ret);
    }

    template <typename F>
    Future<typename std::result_of<F()>::type>
    async(F f)
    {
      return AsyncHelper(f);
    }

    template <typename T>
    Future<std::vector<T> >
    when_all(std::vector<Future<T> >& vec)
    {
      static_assert(std::is_default_constructible<T>::value,
                    "Future type is not default constructible");
      if (vec.empty()) {
        return make_future<std::vector<T> >();
      }
      auto retvec = new std::vector<T>(vec.size());
      auto count = new std::atomic_size_t{vec.size()};
      auto promise = new Promise<std::vector<T> >();
      for (int i = 0; i < vec.size(); ++i) {
        vec[i].OnSuccess([=](T val){
            (*retvec)[i] = std::move(val);
            if (count->fetch_sub(1) == 1) {
              //we are the last one to fulfill it
              promise->Set(std::move(*retvec));
              delete retvec;
              delete count;
              delete promise;
            }
          });
      }
      return promise->GetFuture();
    }

    template <typename T>
    SharedData<T>::SharedData() : state_{PENDING} {}

    //perfect construct T
    template <typename T>
    template <typename ...Args>
    SharedData<T>::SharedData(make_future_tag tag, Args&&... args)
      : state_{FULFILLED}, val_(std::forward<Args>(args)...)
    {
    }

    template <typename T>
    void
    SharedData<T>::Set(const T& val)
    {
      assert(state_ == PENDING);
      {
        mutables_.Lock();
        val_ = val;
        std::atomic_thread_fence(std::memory_order_release);
        state_ = FULFILLED;
        mutables_.Unlock();
      }
      for (const auto& f : success_funcs_) {
        event_manager->Async(std::bind(std::move(f), val_));
      }
    }

    template <typename T>
    void
    SharedData<T>::Set(T&& val)
    {
      assert(state_ == PENDING);
      {
        mutables_.Lock();
        val_ = std::move(val);
        std::atomic_thread_fence(std::memory_order_release);
        state_ = FULFILLED;
        mutables_.Unlock();
      }
      for (const auto& f : success_funcs_) {
        event_manager->Async(std::bind(std::move(f), val_));
      }
    }

    template <typename T>
    void
    SharedData<T>::Set(Future<T> fut)
    {
    }

    template <typename T>
    void
    SharedData<T>::SetException(const std::exception_ptr& exception)
    {
      assert(state_ == PENDING);
      {
        mutables_.Lock();
        exception_ = exception;
        std::atomic_thread_fence(std::memory_order_release);
        state_ = FAILED;
        mutables_.Unlock();
      }
      for (const auto& f : failure_funcs_) {
        event_manager->Async(std::bind(std::move(f), exception_));
      }
    }

    // template <typename T>
    // class SharedData<T&> {
    // public:
    //   SharedData() : state_{PENDING} {}
    //   // no copy
    //   SharedData(const SharedData&) = delete;

    //   SharedData(T& ref) : state_{FULFILLED}, val_{&ref} {}

    //   // no copy
    //   SharedData& operator=(const SharedData&) = delete;

    //   void Set(T& ref)
    //   {
    //     assert(state_ == PENDING);
    //     {
    //       std::lock_guard<std::mutex> lock(mutables_);
    //       val_ = &ref;
    //       std::atomic_thread_fence(std::memory_order_release);
    //       state_ = FULFILLED;
    //     }
    //     for (const auto& f : success_funcs_) {
    //       event_manager->Async(std::bind(std::move(f), std::ref(*val_)));
    //     }
    //   }

    //   void SetException(const std::exception_ptr& exception)
    //   {
    //     assert(state_ == PENDING);
    //     {
    //       std::lock_guard<std::mutex> lock(mutables_);
    //       exception_ = exception;
    //       std::atomic_thread_fence(std::memory_order_release);
    //       state_ = FAILED;
    //     }
    //     for (const auto& f : failure_funcs_) {
    //       event_manager->Async(std::bind(std::move(f), exception_));
    //     }
    //   }

    //   std::mutex mutables_;

    //   static constexpr unsigned PENDING = 0;
    //   static constexpr unsigned FULFILLED = 1;
    //   static constexpr unsigned FAILED = 2;

    //   std::atomic_uint state_;

    //   T* val_;
    //   std::exception_ptr exception_;

    //   std::forward_list<std::function<void(T&)> > success_funcs_;
    //   std::forward_list<std::function<void(std::exception_ptr)> > failure_funcs_;
    // };

    template <typename T, typename F>
    Future<typename std::result_of<F(T)>::type>
    FutureSuccessHelper(F f,
                        std::forward_list<std::function<void(T)> >& list,
                        typename std::enable_if<!std::is_void<typename std::result_of<F(T)>
                        ::type>::value>::type* = 0,
                        typename std::enable_if<!std::is_void<T>::value>::type* = 0)
    {
      auto promise = new Promise<typename std::result_of<F(T)>::type>;
      auto ret = promise->GetFuture();
      list.emplace_front([=](T val) {
          try {
            promise->Set(f(std::move(val)));
          } catch (...) {
            promise->SetException(std::current_exception());
          }
          delete promise;
        });
      return std::move(ret);
    }

    template <typename T, typename F>
    Future<void>
    FutureSuccessHelper(F f,
                        std::forward_list<std::function<void(T)> >& list,
                        typename std::enable_if<std::is_void<
                        typename std::result_of<F(T)>::type>::value>::type* = 0,
                        typename std::enable_if<!std::is_void<T>::value>::type* = 0)
    {
      auto promise = new Promise<void>;
      auto ret = promise->GetFuture();
      list.emplace_front([=](T val) {
          try {
            f(val);
            promise->Set();
          } catch (...) {
            promise->SetException(std::current_exception());
          }
          delete promise;
        });
      return std::move(ret);
    }

    template <typename F>
    Future<typename std::result_of<F()>::type>
    VoidFutureSuccessHelper(F f,
                            std::forward_list<std::function<void()> >& list,
                            typename std::enable_if<!std::is_void<typename std::result_of<F()>
                            ::type>::value>::type* = 0)
    {
      auto promise = new Promise<typename std::result_of<F()>::type>;
      auto ret = promise->GetFuture();
      list.emplace_front([=]() {
          try {
            promise->Set(f());
          } catch (...) {
            promise->SetException(std::current_exception());
          }
          delete promise;
        });
      return std::move(ret);
    }

    template <typename F>
    Future<void>
    VoidFutureSuccessHelper(F f,
                            std::forward_list<std::function<void()> >& list,
                            typename std::enable_if<std::is_void<
                            typename std::result_of<F()>::type>::value>::type* = 0)
    {
      auto promise = new Promise<void>;
      auto ret = promise->GetFuture();
      list.emplace_front([=]() {
          try {
            f();
            promise->Set();
          } catch (...) {
            promise->SetException(std::current_exception());
          }
          delete promise;
        });
      return std::move(ret);
    }

    template <typename F>
    Future<typename std::result_of<F(std::exception_ptr)>::type>
    FutureFailHelper(F f,
                     std::forward_list<std::function
                     <void(std::exception_ptr)> >& list,
                     typename std::enable_if<!std::is_void<typename std::result_of
                     <F(std::exception_ptr)>::type>::value>::type* = 0)
    {
      auto promise = new Promise<typename std::result_of
                                 <F(std::exception_ptr)>::type>;
      auto ret = promise->GetFuture();
      list.emplace_front([=](std::exception_ptr exception) {
          try {
            promise->Set(f(exception));
          } catch (...) {
            promise->SetException(std::current_exception());
          }
          delete promise;
        });
      return std::move(ret);
    }

    template <typename F>
    Future<void>
    FutureFailHelper(F f,
                     std::forward_list<std::function
                     <void(std::exception_ptr)> >& list,
                     typename std::enable_if<std::is_void<typename std::result_of
                     <F(std::exception_ptr)>::type>::value>::type* = 0)
    {
      auto promise = new Promise<void>;
      auto ret = promise->GetFuture();
      list.emplace_front([=](std::exception_ptr exception) {
          try {
            f(exception);
            promise->Set();
          } catch (...) {
            promise->SetException(std::current_exception());
          }
          delete promise;
        });
      return std::move(ret);
    }


    class UnfulfilledFuture : public std::exception {
      virtual const char* what() const noexcept override
      {
        return "Attempted to Get() an unfulfilled Future";
      }
    };

    class UnfailedFuture : public std::exception {
      virtual const char* what() const noexcept override
      {
        return "Attempted to GetException() an unfailed Future";
      }
    };

    template <typename T>
    template <typename ...Args>
    Future<T>::Future(make_future_tag tag, Args&&... args) :
      s_{std::make_shared<SharedData<T> >
        (tag, std::forward<Args>(args)...)}
    {
    }

    template <typename T>
    template <typename F> //FIXME: flatten return
    Future<typename std::result_of<F(T)>::type>
    Future<T>::OnSuccess(F f)
    {
      s_->mutables_.Lock();
      switch (s_->state_.load()) {
      case SharedData<T>::PENDING:
      case SharedData<T>::FAILED:
        {
          auto&& fut = FutureSuccessHelper(f, s_->success_funcs_);
          s_->mutables_.Unlock();
          return fut;
          break;
        }
      case SharedData<T>::FULFILLED:
        {
          auto&& fut = async(std::bind(std::move(f), s_->val_));
          s_->mutables_.Unlock();
          return fut;
          break;
        }
      }
      throw std::runtime_error("Invalid Future state");
    }

    template <typename T>
    template <typename F> //FIXME: flatten return
    Future<typename std::result_of<F(std::exception_ptr)>::type>
    Future<T>::OnFailure(F f)
    {
      s_->mutables_.Lock();
      switch (s_->state_.load()) {
      case SharedData<T>::PENDING:
      case SharedData<T>::FULFILLED:
        {
        auto&& fut = FutureFailHelper(f, s_->failure_funcs_);
        s_->mutables_.Unlock();
        return fut;
        break;
        }
      case SharedData<T>::FAILED:
        {
          auto&& fut = async(std::bind(std::move(f), s_->exception_));
          s_->mutables_.Unlock();
          return fut;
          break;
        }
      }
    }

    template <typename T>
    T
    Future<T>::Get() const
    {
      switch (s_->state_.load()) {
      case SharedData<T>::FAILED:
      case SharedData<T>::PENDING:
        throw UnfulfilledFuture();
        break;
      case SharedData<T>::FULFILLED:
        std::atomic_thread_fence(std::memory_order_acquire);
        return s_->val_;
        break;
      }
      assert(0);
    }

    template <typename T>
    std::exception_ptr
    Future<T>::GetException() const
    {
      switch (s_->state_.load()) {
      case SharedData<T>::FULFILLED:
      case SharedData<T>::PENDING:
        throw UnfailedFuture();
        break;
      case SharedData<T>::FAILED:
        std::atomic_thread_fence(std::memory_order_acquire);
        return s_->exception_;
        break;
      }
      assert(0);
    }

    template <typename T>
    bool
    Future<T>::Fulfilled() const
    {
      return s_->state_ == SharedData<T>::FULFILLED;
    }

    template <typename T>
    bool
    Future<T>::Failed() const
    {
      return s_->state_ == SharedData<T>::FAILED;
    }

    template <typename F> //FIXME: flatten return
    Future<typename std::result_of<F()>::type>
    Future<void>::OnSuccess(F f)
    {
      s_->mutables_.Lock();
      switch (s_->state_.load()) {
      case SharedData<void>::PENDING:
      case SharedData<void>::FAILED:
        {
          auto&& fut = VoidFutureSuccessHelper(f, s_->success_funcs_);
          s_->mutables_.Unlock();
          return fut;
          break;
        }
      case SharedData<void>::FULFILLED:
        {
          auto&& fut = async(std::move(f));
          s_->mutables_.Unlock();
          return fut;
          break;
        }
      }
      assert(0);
    }

    template <typename F> //FIXME: flatten return
    Future<typename std::result_of<F(std::exception_ptr)>::type>
    Future<void>::OnFailure(F f)
    {
      s_->mutables_.Lock();
      switch (s_->state_.load()) {
      case SharedData<void>::PENDING:
      case SharedData<void>::FULFILLED:
        {
        auto&& fut = FutureFailHelper(f, s_->failure_funcs_);
        s_->mutables_.Unlock();
        return fut;
        break;
        }
      case SharedData<void>::FAILED:
        {
        auto&& fut = async(std::bind(std::move(f), s_->exception_));
        s_->mutables_.Unlock();
        return fut;
        break;
        }
      }
      assert(0);
    }

    template <typename T>
    Promise<T>::Promise() :
      i_{std::make_shared<SharedData<T> >()} {}

    template <typename T>
    void
    Promise<T>::Set(const T& val)
    {
      i_->Set(val);
    }

    template <typename T>
    void
    Promise<T>::Set(T&& val)
    {
      i_->Set(std::move(val));
    }

    template <typename T>
    void
    Promise<T>::Set(Future<T> fut)
    {
      assert(i_->state_ == SharedData<T>::PENDING);
      if (fut.Fulfilled()) {
        i_->Set(fut.Get());
      } else if (fut.Failed()) {
        i_->SetException(fut.GetException());
      } else {
        //Kludge to pass member variable by value
        fut.OnSuccess(std::bind([](std::shared_ptr<SharedData<T> > ptr, T val) {
              ptr->Set(std::move(val));
            }, i_, std::placeholders::_1));
        //Kludge to pass member variable by value
        fut.OnFailure(std::bind([](std::shared_ptr<SharedData<T> > ptr,
                                   std::exception_ptr eptr) {
                                  ptr->SetException(std::move(eptr));
                                }, i_, std::placeholders::_1));
      }
    }

    template <typename T>
    void
    Promise<T>::SetException(const std::exception_ptr& exception)
    {
      i_->SetException(exception);
    }

    template <typename T>
    Future<T>
    Promise<T>::GetFuture()
    {
      return Future<T>(i_);
    }
  }

  using future_internal::Future;
  using future_internal::Promise;
  using future_internal::UnfulfilledFuture;
  using future_internal::UnfailedFuture;
  using future_internal::async;
  using future_internal::make_future;
  using future_internal::when_all;
}
#endif
