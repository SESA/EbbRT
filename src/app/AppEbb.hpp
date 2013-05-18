#ifndef EBBRT_APP_APPEBB_HPP
#define EBBRT_APP_APPEBB_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class App : public EbbRep {
  public:
    virtual void Start() = 0;
  };
  extern char app_id_resv __attribute__ ((section ("static_ebb_ids")));
  extern "C" char static_ebb_ids_start[];
  const Ebb<App> app_ebb =
    Ebb<App>(static_cast<EbbId>(&app_id_resv -
                                static_ebb_ids_start));
}

#endif
