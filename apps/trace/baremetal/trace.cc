//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Debug.h>
#include <ebbrt/Trace.h>

void AppMain() { 
  
  ebbrt::trace::Disable();
  ebbrt::trace::Dump();
  ebbrt::kabort();

}
