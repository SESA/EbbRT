#ifndef EBBRT_MISC_VTABLE_HPP
#define EBBRT_MISC_VTABLE_HPP

namespace ebbrt {
  template <class T, typename F>
  ptrdiff_t vtable_index(F f)
  {
    struct VTableCounter
    {
      virtual ptrdiff_t Get0() { return 0; }
      virtual ptrdiff_t Get1() { return 1; }
      virtual ptrdiff_t Get2() { return 2; }
      virtual ptrdiff_t Get3() { return 3; }
      virtual ptrdiff_t Get4() { return 4; }
      virtual ptrdiff_t Get5() { return 5; }
      virtual ptrdiff_t Get6() { return 6; }
      virtual ptrdiff_t Get7() { return 7; }
      virtual ptrdiff_t Get8() { return 8; }
      virtual ptrdiff_t Get9() { return 9; }
    } vt;

    auto t = reinterpret_cast<T*>(&vt);
    auto get_index = reinterpret_cast<int (T::*)()>(f);
    return (t->*get_index)();
  }
}

#endif
