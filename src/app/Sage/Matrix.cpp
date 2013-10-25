#include <cmath>
#include <unordered_map>

#include "ebb/SharedRoot.hpp"
#include "src/app/Sage/Matrix.hpp"

#ifdef __linux__
std::unordered_map<ebbrt::EbbId, size_t> size_map;

void ebbrt::Matrix::SetSize(EbbId id, size_t size) {
  size_map[id] = size;
}
#endif

ebbrt::EbbRoot *ebbrt::Matrix::ConstructRoot() {
  return new SharedRoot<Matrix>();
}

#ifdef __linux__
#include <iostream>
#endif

ebbrt::Matrix::Matrix(EbbId id) : EbbRep(id) {
#ifdef __linux__
  size_ = size_map[id];
  auto bytes_needed = size_ * size_ * 8;
  int nodes = std::ceil((double)bytes_needed / 512000000);
  std::cout << "Allocate " << nodes << " nodes" << std::endl;
#endif
}

void ebbrt::Matrix::Randomize() {}
