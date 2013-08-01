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

#include "ebb/EventManager/Future.hpp"

namespace ebbrt {
  namespace future_internal {

    SharedData<void>::SharedData() : state_{PENDING} {}

    void
    SharedData<void>::Set()
    {
      state_ = FULFILLED;
      for (const auto& f : success_funcs_) {
        event_manager->Async(std::move(f));
      }
    }

    void
    SharedData<void>::SetException(const std::exception_ptr& exception)
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

    std::exception_ptr
    Future<void>::GetException() const
    {
      switch (s_->state_.load()) {
      case SharedData<void>::FULFILLED:
      case SharedData<void>::PENDING:
        throw UnfailedFuture();
        break;
      case SharedData<void>::FAILED:
        std::atomic_thread_fence(std::memory_order_acquire);
        return s_->exception_;
        break;
      }
      throw std::runtime_error("Invalid Future state");
    }

    bool
    Future<void>::Fulfilled() const
    {
      return s_->state_ == SharedData<void>::FULFILLED;
    }

    bool
    Future<void>::Failed() const
    {
      return s_->state_ == SharedData<void>::FAILED;
    }

    Promise<void>::Promise() :
      i_{std::make_shared<SharedData<void> >()} {}

    void
    Promise<void>::Set()
    {
      i_->Set();
    }

    void
    Promise<void>::SetException(const std::exception_ptr& exception)
    {
      i_->SetException(exception);
    }

    Future<void>
    Promise<void>::GetFuture()
    {
      return Future<void>(i_);
    }
  }
}
