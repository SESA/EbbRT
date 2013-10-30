#include <cmath>
#include <cstdio>
#include <random>
#include <unordered_map>

#include "app/app.hpp"
#include "ebb/SharedRoot.hpp"
#include "app/Sage/LocalMatrix.hpp"

namespace {
std::unordered_map<ebbrt::EbbId, size_t> size_map;
}
void ebbrt::LocalMatrix::SetSize(EbbId id, size_t size) { size_map[id] = size; }

ebbrt::EbbRoot *ebbrt::LocalMatrix::ConstructRoot() {
  return new SharedRoot<LocalMatrix>();
}

ebbrt::LocalMatrix::LocalMatrix(EbbId id)
    : EbbRep(id), size_(size_map[id]), matrix_(size_, size_) {}

ebbrt::Future<void>
ebbrt::LocalMatrix::Randomize() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(-1, 1);
  for (auto& elem : matrix_) {
    elem = dis(gen);
  }
  return make_ready_future<void>();
}

ebbrt::Future<double>
ebbrt::LocalMatrix::Get(size_t row, size_t column) {
  return make_ready_future<double>(matrix_(row, column));
}

ebbrt::Future<void>
ebbrt::LocalMatrix::Set(size_t row, size_t column, double value) {
  matrix_(row, column) = value;
  return make_ready_future<void>();
}

ebbrt::Future<double>
ebbrt::LocalMatrix::Sum() {
  return make_ready_future<double>(matrix_.sum());
}
