//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_BUFFER_H_
#define COMMON_SRC_INCLUDE_EBBRT_BUFFER_H_

#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <utility>

namespace ebbrt {
class Buffer {
 public:
  Buffer()
      : next_(nullptr), buffer_(nullptr), free_(nullptr), total_size_(0),
        size_(0), offset_(0), length_(0) {}
  explicit Buffer(size_t size)
      : next_(nullptr), total_size_(size), size_(size), offset_(0), length_(1) {
    buffer_ = std::malloc(size);
    if (buffer_ == nullptr)
      throw std::bad_alloc();
    free_ = [](void* ptr, size_t size) { std::free(ptr); };  // NOLINT
  }
  Buffer(void* p, size_t size, std::function<void(void*, size_t)> f = nullptr)
      : next_(nullptr), buffer_(p), free_(std::move(f)), total_size_(size),
        size_(size), offset_(0), length_(1) {}
  Buffer(const Buffer&) = delete;
  Buffer(Buffer&&) = default;

  virtual ~Buffer() {
    if (!free_)
      return;

    auto p = static_cast<void*>(static_cast<char*>(buffer_) - offset_);
    auto l = size_ + offset_;
    free_(p, l);
  }

  Buffer& operator=(const Buffer&) = delete;
  Buffer& operator=(Buffer&&) = default;

  void* data() { return static_cast<void*>(buffer_); }
  const void* data() const { return static_cast<const void*>(buffer_); }

  size_t size() const { return size_; }
  size_t total_size() const { return total_size_; }
  size_t length() const { return length_; }

  Buffer& operator+=(size_t sz) {
    buffer_ = static_cast<void*>(static_cast<char*>(buffer_) + sz);
    total_size_ -= sz;
    size_ -= sz;
    offset_ += sz;
    return *this;
  }

  std::pair<void*, size_t> release() {
    auto p = static_cast<void*>(static_cast<char*>(buffer_) - offset_);
    buffer_ = nullptr;
    free_ = nullptr;
    return std::make_pair(p, size_ + offset_);
  }

 private:
  std::shared_ptr<Buffer> next_;
  void* buffer_;
  std::function<void(void*, size_t)> free_;
  size_t total_size_;
  size_t size_;
  size_t offset_;
  size_t length_;

 public:
  class iterator {
   public:
    typedef ptrdiff_t difference_type;
    typedef std::forward_iterator_tag iterator_category;
    typedef Buffer value_type;
    typedef Buffer* pointer;
    typedef Buffer& reference;

    iterator() : p_() {}
    explicit iterator(Buffer* p) : p_(p) {}
    reference operator*() const { return *p_; }
    pointer operator->() const { return p_; }
    iterator& operator++() {
      p_ = p_->next_.get();
      return *this;
    }
    bool operator==(const iterator& other) const { return p_ == other.p_; }
    bool operator!=(const iterator& other) const { return p_ != other.p_; }

   private:
    Buffer* p_;
  };

  class const_iterator {
   public:
    typedef ptrdiff_t difference_type;
    typedef std::forward_iterator_tag iterator_category;
    typedef const Buffer value_type;
    typedef const Buffer* pointer;
    typedef const Buffer& reference;

    const_iterator() : p_() {}
    explicit const_iterator(const Buffer* p) : p_(p) {}
    reference operator*() const { return *p_; }
    pointer operator->() const { return p_; }
    const_iterator& operator++() {
      p_ = p_->next_.get();
      return *this;
    }
    bool operator==(const const_iterator& other) const {
      return p_ == other.p_;
    }
    bool operator!=(const const_iterator& other) const {
      return p_ != other.p_;
    }

   private:
    const Buffer* p_;
  };

  class DataPointer {
   public:
    explicit DataPointer(Buffer* p) : p_(p), offset_(0) {}
    void* Addr() {
      if (!p_ || p_->size() - offset_ == 0)
        throw std::runtime_error("DataPointer::Addr() past end of buffer");

      return static_cast<void*>(static_cast<char*>(p_->data()) + offset_);
    }

    template <typename T> T& Get() {
      if (!p_)
        throw std::runtime_error("DataPointer::Get() past end of buffer");

      if (p_->size() - offset_ < sizeof(T))
        throw std::runtime_error("DataPointer::Get() asked to straddle buffer");

      auto ret = reinterpret_cast<T*>(static_cast<char*>(p_->data()) + offset_);
      Advance(sizeof(T));
      return *ret;
    }

    void Advance(size_t size) {
      if (!p_)
        throw std::runtime_error("DataPointer::Advance() past end of buffer");

      auto remainder = p_->size() - offset_;
      if (remainder > size) {
        offset_ += size;
      } else {
        p_ = p_->next_.get();
        offset_ = remainder - size;
      }
    }

   private:
    Buffer* p_;
    size_t offset_;
  };

  DataPointer GetDataPointer() { return DataPointer(this); }

  iterator begin() { return iterator(this); }
  const_iterator begin() const { return const_iterator(this); }

  iterator end() { return iterator(nullptr); }
  const_iterator end() const { return const_iterator(nullptr); }

  template <typename... Args> void emplace_back(Args&&... args) {
    auto b = std::make_shared<Buffer>(std::forward<Args>(args)...);
    append(std::move(b));
  }

  void append(std::shared_ptr<Buffer> b) {
    auto it = begin();
    size_t i = 0;
    auto len = length_;
    while (i < (len - 1)) {
      it->total_size_ += b->total_size();
      it->length_ += b->length();
      ++i;
      ++it;
    }
    it->total_size_ += b->total_size();
    it->length_ += b->length();
    it->next_ = std::move(b);
  }
};
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_BUFFER_H_
