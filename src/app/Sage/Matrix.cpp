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
std::unordered_map<ebbrt::EbbId, size_t> size_map;

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
  std::ofstream outfile{ "/tmp/matrixconfig", std::ofstream::binary };
  outfile.write(outptr, fdt_totalsize(outptr));
  outfile.close();
  auto fp = popen("khinfo MatrixPool", "r");
  char out[1024];
  for (size_t i = 0; i < nodes_; ++i) {
    assert(fgets(out, 1024, fp) != nullptr);
    std::string ip = out;
    if (!ip.empty() && ip[ip.length() - 1] == '\n') {
      ip.erase(ip.length() - 1);
    }
    node_allocator->Allocate(ip, "/home/dschatz/Work/SESA/EbbRT/newbuild/bare/"
                                 "src/app/Sage/SageMatrix.elf32",
                             "/tmp/matrixconfig");
  }
#else
  size_ = 0;
#endif
}

namespace {
enum message_type {
  CONNECT,
  RANDOMIZE,
  RANDOMIZE_COMPLETE
};

struct connect_message {
  message_type type;
};

struct randomize_message {
  message_type type;
  uint32_t op_id;
  size_t xmin;
  size_t xmax;
  size_t ymin;
  size_t ymax;
};

struct randomize_complete_message {
  message_type type;
  uint32_t op_id;
};
}

#ifdef __linux__
ebbrt::Future<void> ebbrt::Matrix::Randomize() {
  auto op_id = op_id_++;
  auto f = [this, op_id]() {
    size_t x = 0;
    size_t y = 0;
    for (const auto &backend : backends_) {
      size_t x_max = std::min(x + MAX_DIM, size_);
      size_t y_max = std::min(y + MAX_DIM, size_);
      randomize_message msg{ RANDOMIZE, op_id, x, x_max, y, y_max };
      x = x_max;
      if (x == size_) {
        y = y_max;
        x = 0;
      }
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
#endif

void ebbrt::Matrix::Connect() {
#ifdef __ebbrt__
  // FIXME: get from config
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
    xmin_ = randomize->xmin;
    xmax_ = randomize->xmax;
    ymin_ = randomize->ymin;
    ymax_ = randomize->ymax;
    size_t x_size = xmax_ - xmin_;
    size_t y_size = ymax_ - ymin_;
    matrix_ = new double *[x_size];
    for (size_t i = 0; i < x_size; ++i) {
      matrix_[i] = new double[y_size];
    }
    char strbuf[80];
    sprintf(strbuf, "Randomizing X: %lu -> %lu Y: %lu -> %lu\n", xmin_, xmax_,
            ymin_, ymax_);
    lrt::console::write(strbuf);
    std::mt19937 gen(15);
    std::uniform_real_distribution<> dis(-1, 1);
    for (size_t i = 0; i < x_size; ++i) {
      for (size_t j = 0; j < y_size; ++j) {
        matrix_[i][j] = dis(gen);
      }
    }
    randomize_complete_message msg{ RANDOMIZE_COMPLETE, randomize->op_id };
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
