#ifndef EBBRT_APP_BARE_APPEBB_HPP
#define EBBRT_APP_BARE_APPEBB_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class App : public EbbRep {
  public:
    virtual void Start() = 0;
  };
  char app_id_resv = 0;
  const Ebb<App> app_ebb =
    Ebb<App>(static_cast<EbbId>(app_id_resv++));
}

#endif
