//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_SLABOBJECT_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_SLABOBJECT_H_

#include <boost/intrusive/slist.hpp>

namespace ebbrt {
class FreeObject {
 public:
  void* addr() { return this; }

  boost::intrusive::slist_member_hook<> member_hook_;
};

typedef boost::intrusive::slist<
    FreeObject,
    boost::intrusive::cache_last<true>,
    boost::intrusive::member_hook<FreeObject,
                                  boost::intrusive::slist_member_hook<>,
                                  &FreeObject::member_hook_> > FreeObjectList;

typedef boost::intrusive::slist< // NOLINT
    FreeObject,
    boost::intrusive::constant_time_size<false>,
    boost::intrusive::member_hook<FreeObject,
                                  boost::intrusive::slist_member_hook<>,
                                  &FreeObject::member_hook_> >
    CompactFreeObjectList;
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_SLABOBJECT_H_
