#ifndef EBBRT_EBB_CONSOLE_CONSOLE_HPP
#define EBBRT_EBB_CONSOLE_CONSOLE_HPP

#include <functional>

#include "ebb/ebb.hpp"

namespace ebbrt {
  class Console : public EbbRep {
  public:
    static EbbRoot* ConstructRoot();
    virtual void Write(const char* str,
                       std::function<void()> cb = nullptr);
  };
  extern char console_id_resv
  __attribute__ ((section ("static_ebb_ids")));
  extern "C" char static_ebb_ids_start[];
  const EbbRef<Console> console =
    EbbRef<Console>(static_cast<EbbId>(&console_id_resv -
                                       static_ebb_ids_start));
}
#endif
