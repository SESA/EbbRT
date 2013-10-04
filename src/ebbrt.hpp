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
#ifndef EBBRT_EBBRT_HPP
#define EBBRT_EBBRT_HPP

#ifdef __bg__
#include <poll.h>
#include <vector>
#endif

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stack>
#include <string>
#include <unordered_map>

#include "lrt/event.hpp"
#include "lrt/event_impl.hpp"
#include "lrt/EbbId.hpp"
#include "lrt/Location.hpp"
#include "lrt/trans_impl.hpp"

//#include "ebb/EventManager/EventManager.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      class RootBinding;
      class EbbRep;
      class EbbRoot;
      template<class U> class EbbRef;
    }
  }
  /**
   A runtime instance,
   Members of this class can be used to construct Context%s
  */
  class EbbRT {
  public:
    EbbRT();
    EbbRT(void* config);
    /**
     * Configuration (flattened device tree) pointer 
     */
    void *fdt_;
  private:
    friend class Context;
    friend const lrt::trans::RootBinding&
    lrt::trans::initial_root_table(unsigned);
    friend void lrt::event::_event_interrupt(uint8_t interrupt);
    friend void lrt::trans::install_miss_handler(lrt::trans::EbbRoot*);
    friend bool lrt::trans::_trans_precall(Args*,
                                           ptrdiff_t,
                                           lrt::trans::FuncRet*);
    friend void* lrt::trans::_trans_postcall(void*);
    friend class lrt::trans::InitRoot;

    /**
     * To be used by Context%s at construction time
     */
    lrt::event::Location AllocateLocation();

    /**
     * Controls access to initialized_
     */
    std::mutex init_lock_;
    /**
     * Controls access to initialized_
     */
    std::condition_variable init_cv_;
    /**
     * Defines whether the EbbRT has been initialized.
     * This must be done within a context and so it cannot be done in
     * the constructor
     */
    bool initialized_;
    /**
     * Used to allocate locations
     */
    std::atomic<lrt::event::Location> next_id_;
    /**
     * The initial, static table of roots that is populated at
    initialization time
    */
    lrt::trans::RootBinding* initial_root_table_;
    /**
     * The root responsible for finding per Ebb roots
     */
    lrt::trans::EbbRoot* miss_handler_;
  };

  /**
   The execution within an EbbRT.
   This is the way for an application to transition into being able to
   make invocations on Ebbs and receive events from the EbbRT
  */
  class Context {
  public:
    /** Construct the Context within an EbbRT instance.
        @param[in] instance The EbbRT this context will belong to
     */
    Context(EbbRT& instance);
    /** Activate Context - Ebb calls can only be made on an active context
     */
    void Activate();
    /** Deactivate Context
     */
    void Deactivate();
    /** Dispatch events
        @param[in] count the number of events to dispatch until
        returning. If -1, then do not return */
    void Loop(int count);
    /** Breaks a running event loop.
        Any currently executing events will run before the loop breaks
    */
    void Break();

    /** The instance this context belongs to */
    EbbRT& instance_;
  private:
    template<class U> friend class lrt::trans::EbbRef;
    friend void lrt::trans::cache_rep(lrt::trans::EbbId, lrt::trans::EbbRep*);
    friend const lrt::trans::RootBinding&
    lrt::trans::initial_root_table(unsigned);
    friend void lrt::trans::install_miss_handler(lrt::trans::EbbRoot*);
    friend void lrt::event::_event_altstack_push(uintptr_t);
    friend uintptr_t lrt::event::_event_altstack_pop();
#ifdef __bg__
    friend void lrt::event::register_function(std::function<int()>);
#endif
    friend bool lrt::trans::_trans_precall(Args*,
                                           ptrdiff_t,
                                           lrt::trans::FuncRet*);
    friend void* lrt::trans::_trans_postcall(void*);
    friend class lrt::trans::InitRoot;
    friend lrt::event::Location lrt::event::get_location();

    /** The location of this context */
    lrt::event::Location location_;
    /** The local table storing per location ebb reps */
    std::unordered_map<lrt::trans::EbbId, lrt::trans::EbbRep*> local_table_;
    /** In case of a miss, the called EbbID is stored here */
    lrt::trans::EbbId last_ebbcall_id_;
    /** The stack used to store miss information */
    std::stack<uintptr_t> altstack_;
    /** Whether or not to break */
    std::atomic<bool> break_loop_;
  };
}

#endif
