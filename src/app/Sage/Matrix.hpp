#pragma once

#include <vector>

#include "ebb/ebb.hpp"
#ifdef __linux__
#include "ebb/EventManager/Future.hpp"
#endif

namespace ebbrt {
class Matrix : public EbbRep {
  size_t size_;
  std::vector<NetworkId> backends_;
  size_t connected_;
  size_t nodes_;
#ifdef __ebbrt__
  double** matrix_;
  size_t xmin_;
  size_t xmax_;
  size_t ymin_;
  size_t ymax_;
#endif
#ifdef __linux__
  bool completed_connect_;
  std::function<void()> on_connect;
  uint32_t op_id_;
  std::unordered_map<unsigned,
                     std::pair<size_t, Promise<void> > > promise_map_;
#endif
public:
  static EbbRoot *ConstructRoot();
  static void SetSize(EbbId id, size_t size);

  Matrix(EbbId id);
#ifdef __linux__
  virtual Future<void> Randomize();
#endif
  virtual void Connect();

  virtual void HandleMessage(NetworkId from, Buffer buf) override;
};
}
