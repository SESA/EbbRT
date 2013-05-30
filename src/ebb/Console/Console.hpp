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
    void HandleMessage(const uint8_t* message,
                       size_t len) override;
  };
  const EbbRef<Console> console =
    EbbRef<Console>(lrt::trans::find_static_ebb_id("Console"));
}
#endif
