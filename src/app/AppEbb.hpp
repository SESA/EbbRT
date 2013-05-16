#ifndef EBBRT_APP_APPEBB_HPP
#define EBBRT_APP_APPEBB_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class App : public EbbRep {
  public:
    virtual void Start() = 0;
  };
  extern Ebb<App> app_ebb;
}

#endif
