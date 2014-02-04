//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Argv.h>
#include <ebbrt/Debug.h>
#include <cstring>

namespace ebbrt {
  namespace argv {
    Data data;

    size_t Init(const char *str) {
      data.len = 0; 
      kprintf("%s: %s\n", __PRETTY_FUNCTION__, str);
      bzero(data.cmdline_string, kCmdLength);
      size_t n = strlcpy(data.cmdline_string,
			 str,
			 kCmdLength);
      if (n>=kCmdLength) {
	kprintf("%s: WARNING: EbbRT command line of length %d  exceeds max allowed"
		"(%d) -- truncated to max\n", __PRETTY_FUNCTION__,
		n, kCmdLength);
      }
      return n;
    }


  }
}

