#pragma once

#include "ebb/ebb.hpp"

namespace ebbrt {
class Matrix : public EbbRep {
  size_t size_;

public:
  static EbbRoot *ConstructRoot();
  static void SetSize(EbbId id, size_t size);

  Matrix(EbbId id);
  virtual void Randomize();
};
}
