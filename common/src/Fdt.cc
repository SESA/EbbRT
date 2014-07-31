//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Fdt.h>

#include <cstdlib>

extern "C" {
#include <libfdt.h>
}

ebbrt::Fdt::Fdt() : ptr_(nullptr, &std::free) {
  size_ = 4096;
  auto fdt_mem = malloc(size_);
  if (fdt_mem == nullptr)
    throw std::bad_alloc();

retry:
  auto err = fdt_create(fdt_mem, size_);
  if (err == -FDT_ERR_NOSPACE) {
    size_ *= 2;
    fdt_mem = realloc(fdt_mem, size_);
    if (fdt_mem == nullptr) {
      throw std::bad_alloc();
    }
    goto retry;
  } else if (err != 0) {
    free(fdt_mem);
    throw std::runtime_error("fdt_create failed");
  }

  err = fdt_finish_reservemap(fdt_mem);
  if (err != 0) {
    free(fdt_mem);
    throw std::runtime_error("fdt_finish_reservemap failed");
  }

  ptr_.reset(fdt_mem);
}

void ebbrt::Fdt::BeginNode(const char* name) {
retry:
  auto err = fdt_begin_node(ptr_.get(), name);
  if (err == -FDT_ERR_NOSPACE) {
    IncreaseSpace();
    goto retry;
  } else if (err != 0) {
    throw std::runtime_error("fdt_begin_node failed");
  }
}

void ebbrt::Fdt::EndNode() {
retry:
  auto err = fdt_end_node(ptr_.get());
  if (err == -FDT_ERR_NOSPACE) {
    IncreaseSpace();
    goto retry;
  } else if (err != 0) {
    throw std::runtime_error("fdt_end failed");
  }
}

void ebbrt::Fdt::CreateProperty(const char* name, uint16_t val) {
  val = cpu_to_fdt16(val);
  auto ptr = static_cast<const char*>(static_cast<const void*>(&val));
  CreateProperty(name, ptr, sizeof(val));
}

void ebbrt::Fdt::CreateProperty(const char* name, uint32_t val) {
  val = cpu_to_fdt32(val);
  auto ptr = static_cast<const char*>(static_cast<const void*>(&val));
  CreateProperty(name, ptr, sizeof(val));
}

void ebbrt::Fdt::CreateProperty(const char* name, uint64_t val) {
  val = cpu_to_fdt64(val);
  auto ptr = static_cast<const char*>(static_cast<const void*>(&val));
  CreateProperty(name, ptr, sizeof(val));
}

void ebbrt::Fdt::CreateProperty(const char* name, const char* val, int len) {
retry:
  auto err = fdt_property(ptr_.get(), name, val, len);
  if (err == -FDT_ERR_NOSPACE) {
    IncreaseSpace();
    goto retry;
  } else if (err != 0) {
    throw std::runtime_error("fdt_property failed");
  }
}

void ebbrt::Fdt::Finish() {
retry:
  auto err = fdt_finish(ptr_.get());
  if (err == -FDT_ERR_NOSPACE) {
    IncreaseSpace();
    goto retry;
  } else if (err != 0) {
    throw std::runtime_error("fdt_finish failed");
  }
}

const char* ebbrt::Fdt::GetAddr() {
  return static_cast<const char*>(ptr_.get());
}

size_t ebbrt::Fdt::GetSize() { return fdt_totalsize(ptr_.get()); }

void ebbrt::Fdt::IncreaseSpace() {
  size_ *= 2;
  auto ptr = std::realloc(ptr_.get(), size_);
  if (ptr == nullptr) {
    throw std::bad_alloc();
  }
  ptr_.reset(ptr);
  auto err = fdt_move(ptr_.get(), ptr_.get(), size_);
  if (err != 0) {
    throw std::runtime_error("fdt_move failed");
  }
}

ebbrt::FdtReader::FdtReader(const void* ptr) : ptr_(ptr) {}

boost::optional<size_t> ebbrt::FdtReader::GetNodeOffset(const char* path) {
  auto err = fdt_path_offset(ptr_, path);
  if (err < 0)
    return boost::optional<size_t>();

  return boost::optional<size_t>(err);
}

namespace {
boost::optional<const void*> GetPropertyData(const void* fdt, size_t offset,
                                             const char* name) {
  auto prop = fdt_get_property(fdt, offset, name, nullptr);
  if (prop == nullptr)
    return boost::optional<const void*>();

  return boost::optional<const void*>(static_cast<const void*>(prop->data));
}
}

boost::optional<uint16_t> ebbrt::FdtReader::GetProperty16(size_t offset,
                                                          const char* name) {
  auto prop_data = GetPropertyData(ptr_, offset, name);
  if (!prop_data)
    return boost::optional<uint16_t>();

  auto val = *static_cast<const uint16_t*>(*prop_data);
  return boost::optional<uint16_t>(fdt16_to_cpu(val));
}

boost::optional<uint32_t> ebbrt::FdtReader::GetProperty32(size_t offset,
                                                          const char* name) {
  auto prop_data = GetPropertyData(ptr_, offset, name);
  if (!prop_data)
    return boost::optional<uint32_t>();

  auto val = *static_cast<const uint32_t*>(*prop_data);
  return boost::optional<uint32_t>(fdt32_to_cpu(val));
}
