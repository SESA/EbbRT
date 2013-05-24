#ifndef EBBRT_EBB_EBB_HPP
#define EBBRT_EBB_EBB_HPP

#include "lrt/boot.hpp"
#include "lrt/event.hpp"
#include "lrt/trans.hpp"

namespace ebbrt {
  using lrt::trans::EbbRef;
  using lrt::trans::EbbId;
  using lrt::trans::NodeId;
  using lrt::trans::EbbRep;
  using lrt::trans::EbbRoot;
  using lrt::event::Location;
  using lrt::event::get_location;
}

#endif
