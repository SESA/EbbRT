#ifndef EBBRT_APP_APPEBB_HPP
#define EBBRT_APP_APPEBB_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class App : public EbbRep {
  public:
    virtual void Start() = 0;
  };

#ifdef LRT_BARE

  extern char app_id_resv __attribute__ ((section ("static_ebb_ids")));
  extern "C" char static_ebb_ids_start[];
  const EbbRef<App> app_ebb =
    EbbRef<App>(static_cast<EbbId>(&app_id_resv -
                                   static_ebb_ids_start));
#elif LRT_ULNX

//  char app_id_resv = 0;
  const EbbRef<App> app_ebb =
    EbbRef<App>(static_cast<EbbId>(0));

#endif
}

#endif
