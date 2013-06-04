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
