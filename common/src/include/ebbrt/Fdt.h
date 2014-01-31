//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_FDT_H_
#define COMMON_SRC_INCLUDE_EBBRT_FDT_H_

#include <memory>

namespace ebbrt {
class Fdt {
 public:
  Fdt();

  // TODO(dschatz): would be nice to use RAII for this
  void BeginNode(const char* name);
  void EndNode();
  void CreateProperty(const char* name, uint16_t val);
  void CreateProperty(const char* name, uint32_t val);
  void CreateProperty(const char* name, uint64_t val);
  void CreateProperty(const char* name, const char* val, int len);
  void Finish();
  const char* GetAddr();
  size_t GetSize();

 private:
  void IncreaseSpace();

  std::unique_ptr<void, void (*)(void*)> ptr_;
  size_t size_;
};

class FdtReader {
 public:
  explicit FdtReader(const void* ptr);

  size_t GetNodeOffset(const char* path);
  uint16_t GetProperty16(size_t node_offset, const char* name);
  uint32_t GetProperty32(size_t node_offset, const char* name);

 private:
  const void* ptr_;
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_FDT_H_
