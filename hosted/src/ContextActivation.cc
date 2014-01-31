//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>

ebbrt::ContextActivation::ContextActivation(Context& c) : c_(c) {
  c.Activate();
}

ebbrt::ContextActivation::~ContextActivation() { c_.Deactivate(); }
