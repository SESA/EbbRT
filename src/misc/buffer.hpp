#ifndef EBBRT_MISC_BUFFER_HPP
#define EBBRT_MISC_BUFFER_HPP

#include <list>

namespace ebbrt {
  typedef std::list<std::pair<const void*, size_t> > BufferList;
}

#endif
