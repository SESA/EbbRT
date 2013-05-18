#ifndef EBBRT_APP_APPEBB_HPP
#define EBBRT_APP_APPEBB_HPP

<<<<<<< HEAD
#ifdef LRT_BARE
#include <src/app/bare/AppEbb.hpp>
#elif LRT_ULNX
#include <src/app/ulnx/AppEbb.hpp>
#endif
=======
#include "ebb/ebb.hpp"

namespace ebbrt {
  class App : public EbbRep {
  public:
    virtual void Start() = 0;
    virtual ~App() {}
  };
  extern char app_id_resv __attribute__ ((section ("static_ebb_ids")));
  extern "C" char static_ebb_ids_start[];
  const Ebb<App> app_ebb =
    Ebb<App>(static_cast<EbbId>(&app_id_resv -
                                static_ebb_ids_start));
}
>>>>>>> Improve PCI and start virtio

#endif
