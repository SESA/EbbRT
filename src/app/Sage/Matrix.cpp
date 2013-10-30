#include <cmath>
#include <cstdio>
#include <random>
#include <unordered_map>

#include "app/app.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/MessageManager/MessageManager.hpp"
#include "src/app/Sage/Matrix.hpp"
#include "ebb/Config/Config.hpp"

#ifdef __linux__
#include "ebb/NodeAllocator/NodeAllocator.hpp"
#include "lib/fdt/libfdt.h"
#endif

#ifdef __linux__
namespace {
std::unordered_map<ebbrt::EbbId, size_t> size_map;
}

void ebbrt::Matrix::SetSize(EbbId id, size_t size) { size_map[id] = size; }
#endif

ebbrt::EbbRoot *ebbrt::Matrix::ConstructRoot() {
  return new SharedRoot<Matrix>();
}

// registers symbol for configuration
__attribute__((constructor(65535))) static void _reg_symbol() {
  ebbrt::app::AddSymbol("Matrix", ebbrt::Matrix::ConstructRoot);
}

#ifdef __linux__
#include <iostream>
#endif

namespace {
constexpr size_t MAX_DIM = 2000;
constexpr size_t MAX_BYTES = MAX_DIM * MAX_DIM * 8;
}

ebbrt::Matrix::Matrix(EbbId id) : EbbRep(id) {
#ifdef __linux__
  completed_connect_ = false;
  op_id_ = 0;
  size_ = size_map[id];
  auto bytes_needed = size_ * size_ * 8;
  nodes_ = std::ceil((double)bytes_needed / MAX_BYTES);
  backends_.reserve(nodes_);
  connected_ = 0;
  config_handle->SetInt32("/ebbs/Matrix", "id", id);
  auto outptr = static_cast<const char *>(ebbrt::config_handle->GetConfig());
  auto tmpfilename = tmpnam(nullptr);
  std::ofstream outfile{ tmpfilename, std::ofstream::binary };
  outfile.write(outptr, fdt_totalsize(outptr));
  outfile.close();

  auto bare_bin = getenv("SAGE_BIN");
  assert(bare_bin != nullptr);
  node_allocator->Allocate(bare_bin, tmpfilename, nodes_);
#else
  size_ = 0;
  matrix_initialized_ = false;
#endif
}

namespace {
enum message_type {
  CONNECT,
  RANDOMIZE,
  RANDOMIZE_COMPLETE,
  GET,
  GET_COMPLETE,
  SET,
  SET_COMPLETE,
  SUM,
  SUM_COMPLETE
};

struct connect_message {
  message_type type;
};

struct randomize_message {
  message_type type;
  uint32_t op_id;
  size_t size;
  size_t index;
};

struct randomize_complete_message {
  message_type type;
  uint32_t op_id;
};

struct get_message {
  message_type type;
  uint32_t op_id;
  size_t size;
  size_t index;
  size_t row;
  size_t column;
};

struct get_complete_message {
  message_type type;
  uint32_t op_id;
  double val;
};

struct set_message {
  message_type type;
  uint32_t op_id;
  size_t size;
  size_t index;
  size_t row;
  size_t column;
  double value;
};

struct set_complete_message {
  message_type type;
  uint32_t op_id;
};

struct sum_message {
  message_type type;
  uint32_t op_id;
  size_t size;
  size_t index;
};

struct sum_complete_message {
  message_type type;
  uint32_t op_id;
  double value;
};
}

#ifdef __linux__
ebbrt::Future<void> ebbrt::Matrix::Randomize() {
  auto op_id = op_id_++;
  auto f = [this, op_id]() {
    size_t index = 0;
    for (const auto &backend : backends_) {
      randomize_message msg{ RANDOMIZE, op_id, size_, index };
      index++;
      auto buf = message_manager->Alloc(sizeof(msg));
      std::memcpy(buf.data(), static_cast<void *>(&msg), sizeof(msg));
      message_manager->Send(backend, ebbid_, std::move(buf));
    }
  };

  auto it = promise_map_.emplace(
      std::piecewise_construct, std::forward_as_tuple(op_id),
      std::forward_as_tuple(std::piecewise_construct, std::forward_as_tuple(0),
                            std::forward_as_tuple()));
  if (completed_connect_) {
    f();
  } else {
    on_connect = f;
  }
  return it.first->second.second.GetFuture();
}

ebbrt::Future<double> ebbrt::Matrix::Get(size_t row, size_t column) {
  auto op_id = op_id_++;
  auto f = [this, op_id, row, column]() {
    auto index_per_row = (size_ + (MAX_DIM - 1)) / MAX_DIM;
    auto index = (row / MAX_DIM) * index_per_row + (column / MAX_DIM);
    get_message msg { GET, op_id, size_, index, row % MAX_DIM, column % MAX_DIM};
    auto buf = message_manager->Alloc(sizeof(msg));
    std::memcpy(buf.data(), static_cast<void *>(&msg), sizeof(msg));
    message_manager->Send(backends_[index], ebbid_, std::move(buf));
  };

  auto it = get_promise_map_.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(op_id),
                                     std::forward_as_tuple());
  if (completed_connect_) {
    f();
  } else {
    on_connect = f;
  }
  return it.first->second.GetFuture();
}

ebbrt::Future<void> ebbrt::Matrix::Set(size_t row, size_t column, double value) {
  auto op_id = op_id_++;
  auto f = [this, op_id, row, column, value]() {
    auto index_per_row = (size_ + (MAX_DIM - 1)) / MAX_DIM;
    auto index = (row / MAX_DIM) * index_per_row + (column / MAX_DIM);
    set_message msg { SET, op_id, size_, index, row % MAX_DIM, column % MAX_DIM, value};
    auto buf = message_manager->Alloc(sizeof(msg));
    std::memcpy(buf.data(), static_cast<void *>(&msg), sizeof(msg));
    message_manager->Send(backends_[index], ebbid_, std::move(buf));
  };

  auto it = set_promise_map_.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(op_id),
                                     std::forward_as_tuple());
  if (completed_connect_) {
    f();
  } else {
    on_connect = f;
  }
  return it.first->second.GetFuture();
}

ebbrt::Future<double> ebbrt::Matrix::Sum() {
  auto op_id = op_id_++;
  auto f = [this, op_id]() {
    size_t index = 0;
    for (const auto &backend : backends_) {
      sum_message msg{ SUM, op_id, size_, index };
      index++;
      auto buf = message_manager->Alloc(sizeof(msg));
      std::memcpy(buf.data(), static_cast<void *>(&msg), sizeof(msg));
      message_manager->Send(backend, ebbid_, std::move(buf));
    }
  };

  Promise<double> prom;
  auto it = sum_promise_map_.emplace(op_id, std::tuple<size_t, double,
                                     Promise<double> >(0, 0, Promise<double>()));
  if (completed_connect_) {
    f();
  } else {
    on_connect = f;
  }
  return std::get<2>(it.first->second).GetFuture();
}
#endif

void ebbrt::Matrix::Connect() {
#ifdef __ebbrt__
  NetworkId frontend;
  size_t pos = 0;
  int count = 0;
  int ip[4];
  std::string s = config_handle->GetString("/", "frontend_ip");
  while ((pos = s.find(".")) != std::string::npos) {
    std::string t = s.substr(0, pos);
    ip[count++] = atoi(t.c_str());
    s.erase(0, pos + 1);
  } ip[count] = atol(s.c_str());

  frontend.addr = ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];

  connect_message msg{ CONNECT };
  auto buf = message_manager->Alloc(sizeof(msg));
  std::memcpy(buf.data(), static_cast<void *>(&msg), sizeof(msg));

  message_manager->Send(frontend, ebbid_, std::move(buf));
#endif
}

#ifdef __ebbrt__
void ebbrt::Matrix::Initialize(size_t size, size_t index) {
  auto index_per_side = (size + (MAX_DIM - 1)) / MAX_DIM;
  auto x_index = index % index_per_side;
  auto y_index = index / index_per_side;
  auto x_len = std::min(size, (x_index + 1) * MAX_DIM) - x_index * MAX_DIM;
  auto y_len = std::min(size, (y_index + 1) * MAX_DIM) - y_index * MAX_DIM;
  matrix_ = matrix<double>(y_len, x_len);
  matrix_initialized_ = true;
}
#endif

void ebbrt::Matrix::HandleMessage(NetworkId from, Buffer buf) {
#ifdef __linux__
  assert(buf.length() >= sizeof(message_type));

  auto op = reinterpret_cast<const message_type *>(buf.data());
  switch (*op) {
  case message_type::CONNECT:
    backends_.push_back(from);
    connected_++;
    if (connected_ == nodes_) {
      completed_connect_ = true;
      if (on_connect) {
        on_connect();
      }
    }
    break;
  case message_type::RANDOMIZE_COMPLETE: {
    auto randomize =
        reinterpret_cast<const randomize_complete_message *>(buf.data());
    promise_map_[randomize->op_id].first++;
    if (promise_map_[randomize->op_id].first == nodes_) {
      promise_map_[randomize->op_id].second.SetValue();
    }
    break;
  }
  case message_type::GET_COMPLETE: {
    auto get = reinterpret_cast<const get_complete_message *>(buf.data());
    get_promise_map_[get->op_id].SetValue(get->val);
    break;
  }
  case message_type::SET_COMPLETE: {
    auto set = reinterpret_cast<const set_complete_message *>(buf.data());
    set_promise_map_[set->op_id].SetValue();
    break;
  }
  case message_type::SUM_COMPLETE: {
    auto sum =
      reinterpret_cast<const sum_complete_message *>(buf.data());
    auto &tup = sum_promise_map_[sum->op_id];
    std::get<0>(tup)++;
    std::get<1>(tup) += sum->value;
    if (std::get<0>(tup) == nodes_) {
      std::get<2>(tup).SetValue(std::get<1>(tup));
    }
    break;
  }
  default:
    assert(0);
    break;
  }
#elif __ebbrt__
  LRT_ASSERT(buf.length() >= sizeof(message_type));

  auto op = reinterpret_cast<const message_type *>(buf.data());
  switch (*op) {
  case message_type::RANDOMIZE: {
    auto randomize = reinterpret_cast<const randomize_message *>(buf.data());
    if (!matrix_initialized_) {
      Initialize(randomize->size, randomize->index);
    }
    // char strbuf[80];
    // sprintf(strbuf, "Randomizing\n");
    // lrt::console::write(strbuf);
    std::mt19937 gen(15);
    std::uniform_real_distribution<> dis(-1, 1);
    for (auto& elem : matrix_) {
      elem = dis(gen);
    }
    randomize_complete_message msg{ RANDOMIZE_COMPLETE, randomize->op_id };
    auto buf = message_manager->Alloc(sizeof(msg));
    std::memcpy(buf.data(), static_cast<void *>(&msg), sizeof(msg));

    message_manager->Send(from, ebbid_, std::move(buf));
    break;
  }
  case message_type::GET: {
    auto get = reinterpret_cast<const get_message *>(buf.data());
    if (!matrix_initialized_) {
      Initialize(get->size, get->index);
    }
    get_complete_message msg{ GET_COMPLETE, get->op_id,
        matrix_(get->row,get->column)};
    auto buf = message_manager->Alloc(sizeof(msg));
    std::memcpy(buf.data(), static_cast<void *>(&msg), sizeof(msg));
    message_manager->Send(from, ebbid_, std::move(buf));
    break;
  }
  case message_type::SET: {
    auto set = reinterpret_cast<const set_message *>(buf.data());
    if (!matrix_initialized_) {
      Initialize(set->size, set->index);
    }
    set_complete_message msg{ SET_COMPLETE, set->op_id };
    auto buf = message_manager->Alloc(sizeof(msg));
    std::memcpy(buf.data(), static_cast<void *>(&msg), sizeof(msg));
    message_manager->Send(from, ebbid_, std::move(buf));
    matrix_(set->row,set->column) = set->value;
    break;
  }
  case message_type::SUM: {
    auto sum = reinterpret_cast<const sum_message *>(buf.data());
    if (!matrix_initialized_) {
      Initialize(sum->size, sum->index);
    }
    sum_complete_message msg{ SUM_COMPLETE, sum->op_id,
        matrix_.sum() };
    auto buf = message_manager->Alloc(sizeof(msg));
    std::memcpy(buf.data(), static_cast<void *>(&msg), sizeof(msg));
    message_manager->Send(from, ebbid_, std::move(buf));
    break;
  }
  default:
    LRT_ASSERT(0);
    break;
  }
#endif
}
