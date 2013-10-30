#pragma once

#include <valarray>
#include <vector>

#include "ebb/ebb.hpp"
#include "ebb/EventManager/Future.hpp"

namespace ebbrt {
class LocalMatrix : public EbbRep {
  template <typename T> class matrix {
    size_t rows_;
    size_t cols_;
    std::valarray<T> data_;

  public:
    matrix(size_t rows = 0, size_t cols = 0)
      : rows_{ rows }, cols_{ cols }, data_(rows * cols) {}
    T &operator()(size_t r, size_t c) { return data_[r * cols_ + c]; }
    const T &operator()(size_t r, size_t c) const {
      return data_[r * cols_ + c];
    }
    typedef T *iterator;
    typedef const T *const_iterator;

    iterator begin() { return &data_[0]; }
    const_iterator begin() const { return &data_[0]; }
    iterator end() { return &data_[rows_ * cols_]; }
    const_iterator end() const { return &data_[rows_ * cols_]; }

    T sum() const { return data_.sum(); }
  };
  size_t size_;
  matrix<double> matrix_;
public:
  static EbbRoot *ConstructRoot();
  static void SetSize(EbbId id, size_t size);

  LocalMatrix(EbbId id);
  virtual Future<void> Randomize();
  virtual Future<double> Get(size_t row, size_t column);
  virtual Future<void> Set(size_t row, size_t column, double value);
  virtual Future<double> Sum();
};
}
