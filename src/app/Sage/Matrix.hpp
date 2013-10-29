#pragma once

#include <valarray>
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
  template <typename T>
  class matrix {
    size_t rows_;
    size_t cols_;
    std::valarray<T> data_;
  public:
    matrix(size_t rows = 0, size_t cols = 0)
      : rows_{rows}, cols_{cols}, data_(rows * cols) {}
    T& operator()(size_t r, size_t c) {
      return data_[r * cols_ + c];
    }
    const T& operator()(size_t r, size_t c) const {
      return data_[r * cols_ + c];
    }
    typedef T* iterator;
    typedef const T* const_iterator;

    iterator begin() { return &data_[0];}
    const_iterator begin() const { return &data_[0];}
    iterator end() { return &data_[rows_ * cols_];}
    const_iterator end() const { return &data_[rows_ * cols_];}
  };
  void Initialize(size_t rows, size_t cols);
  matrix<double> matrix_;
  bool matrix_initialized_;
#endif
#ifdef __linux__
  bool completed_connect_;
  std::function<void()> on_connect;
  uint32_t op_id_;
  std::unordered_map<unsigned,
                     std::pair<size_t, Promise<void> > > promise_map_;
  std::unordered_map<unsigned,
                     Promise<double> > get_promise_map_;
#endif
public:
  static EbbRoot *ConstructRoot();
  static void SetSize(EbbId id, size_t size);

  Matrix(EbbId id);
#ifdef __linux__
  virtual Future<void> Randomize();
  virtual Future<double> Get(size_t row, size_t column);
#elif __ebbrt__
#endif
  virtual void Connect();

  virtual void HandleMessage(NetworkId from, Buffer buf) override;
};
}
