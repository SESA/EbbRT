#ifndef EBBRT_SYNC_COMPILER_HPP
#define EBBRT_SYNC_COMPILER_HPP

namespace ebbrt {
  template<typename T>
  inline T volatile& access_once(T &t)
  {
    return const_cast<T volatile&>(t);
  }
}

#endif
