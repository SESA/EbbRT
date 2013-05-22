#ifndef EBBRT_EBB_EBBMANAGER_PRIMITIVEEBBMANAGER_HPP
#define EBBRT_EBB_EBBMANAGER_PRIMITIVEEBBMANAGER_HPP

#include <map>
#include <unordered_map>

#include "ebb/EbbManager/EbbManager.hpp"
#include "sync/spinlock.hpp"

namespace ebbrt {
  class PrimitiveEbbManager : public EbbManager {
  public:
    PrimitiveEbbManager(std::unordered_map<EbbId, EbbRoot*>& root_table,
                        Spinlock& root_table_lock,
                        std::unordered_map<EbbId, EbbRoot* (*)()>& factory_table,
                        Spinlock& factory_table_lock);
    void CacheRep(EbbId id, EbbRep* rep) override;
    EbbId AllocateId() override;
    void Bind(EbbRoot* (*factory)(), EbbId id) override;
  private:
    virtual void Install() override;
    friend class PrimitiveEbbManagerRoot;
    EbbId next_free_;
    std::unordered_map<EbbId, EbbRoot*>& root_table_;
    Spinlock& root_table_lock_;
    std::unordered_map<EbbId, EbbRoot* (*)()>& factory_table_;
    Spinlock& factory_table_lock_;
  };

  EbbRoot* PrimitiveEbbManagerConstructRoot();
  class PrimitiveEbbManagerRoot : public EbbRoot {
  public:
    bool PreCall(Args* args, ptrdiff_t fnum,
                 lrt::trans::FuncRet* fret, EbbId id) override;
    void* PostCall(void* ret) override;
  private:
    std::map<Location, PrimitiveEbbManager*> reps_;
    Spinlock lock_;
  };
}

#endif
