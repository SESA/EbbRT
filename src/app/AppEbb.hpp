#ifndef EBBRT_APP_APPEBB_HPP
#define EBBRT_APP_APPEBB_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class App : public EbbRep {
  public:
    virtual void Start() = 0;
  };

  const EbbRef<App> app_ebb =
    EbbRef<App>(lrt::trans::find_static_ebb_id("App"));
}

#endif
