//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_BUFFER_H_
#define COMMON_SRC_INCLUDE_EBBRT_BUFFER_H_

#include <cstring>
#include <forward_list>
#include <memory>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>

#ifdef __ebbrt__
#include <ebbrt/Debug.h>
#endif

namespace ebbrt {
namespace detail {
// Is T a unique_ptr<> to a standard-layout type
template <class T, class Enable = void>
struct IsUniquePtrToSL : public std::false_type {};

template <class T, class D>
struct IsUniquePtrToSL<std::unique_ptr<T, D>,
                       typename std::enable_if<std::is_standard_layout<
                           T>::value>::type> : public std::true_type {};
}  // namespace detail
class IOBuf {
 public:
  class ConstIterator;
  class Iterator;

  IOBuf() noexcept;
  explicit IOBuf(size_t capacity, bool zero_memory = false);
  IOBuf(void* buf, size_t capacity, std::function<void(void*)> free_func);
  IOBuf(IOBuf&& other) noexcept;
  IOBuf& operator=(IOBuf&& other) noexcept;

  static std::unique_ptr<IOBuf> Create(size_t capacity,
                                       bool zero_memory = false);

  std::unique_ptr<IOBuf> Clone() const;
  std::unique_ptr<IOBuf> CloneOne() const;

  static std::unique_ptr<IOBuf>
  TakeOwnership(void* buf, size_t capacity,
                std::function<void(void*)> free_func = [](void*) {});

  template <class UniquePtr>
  static typename std::enable_if<detail::IsUniquePtrToSL<UniquePtr>::value,
                                 std::unique_ptr<IOBuf>>::type
  TakeOwnership(UniquePtr&& buf, size_t count = 1);

  static std::unique_ptr<IOBuf> WrapBuffer(const void* buf, size_t size);

  static std::unique_ptr<IOBuf> CopyBuffer(const void* buf, size_t size) {
    auto b = Create(size);
    std::memcpy(b->WritableData(), buf, size);
    return b;
  }

  static std::unique_ptr<IOBuf> CopyBuffer(const std::string& buf) {
    return CopyBuffer(buf.data(), buf.size());
  }

  static std::unique_ptr<IOBuf> Clone(const std::unique_ptr<const IOBuf>& buf);

  static void Destroy(std::unique_ptr<IOBuf>&& data) {
    auto destroyer = std::move(data);
  }

  std::string ToString();

  ~IOBuf();

  bool Empty() const;

  const uint8_t* Data() const { return data_; }

  uint8_t* WritableData() { return data_; }

  const uint8_t* Tail() const { return data_ + length_; }

  uint8_t* WritableTail() { return data_ + length_; }

  size_t Length() const { return length_; }

  const uint8_t* Buffer() const { return buf_.get(); }

  uint8_t* WritableBuffer() { return buf_.get(); }

  const uint8_t* BufferEnd() const { return buf_.get() + capacity_; }

  uint64_t Capacity() const { return capacity_; }

  IOBuf* Next() { return next_; }

  const IOBuf* Next() const { return next_; }

  IOBuf* Prev() { return prev_; }

  const IOBuf* Prev() const { return prev_; }

  void Advance(size_t amount) {
    data_ += amount;
    length_ -= amount;
  }

  void Retreat(size_t amount) {
    data_ -= amount;
    length_ += amount;
  }

  void TrimEnd(size_t amount) { length_ -= amount; }

  bool IsChained() const { return next_ != this; }

  size_t CountChainElements() const;

  size_t ComputeChainDataLength() const;

  void PrependChain(std::unique_ptr<IOBuf>&& iobuf);
  void AppendChain(std::unique_ptr<IOBuf>&& iobuf) {
    next_->PrependChain(std::move(iobuf));
  }

  /* Remove this IOBuf from its current chain and return a unique_ptr to it. Do
   * not call on the head of a chain that is owned already */
  std::unique_ptr<IOBuf> Unlink() {
    next_->prev_ = prev_;
    prev_->next_ = next_;
    prev_ = this;
    next_ = this;
    return std::unique_ptr<IOBuf>(this);
  }

  /* Remove this IOBuf from its current chain and return a unique_ptr to the
     IOBuf that formerly followed it in the chain */
  std::unique_ptr<IOBuf> Pop() {
    IOBuf* next = next_;
    next_->prev_ = prev_;
    prev_->next_ = next_;
    prev_ = this;
    next_ = this;
    return std::unique_ptr<IOBuf>((next == this) ? nullptr : next);
  }

  bool Unique() const {
    const IOBuf* current = this;
    while (true) {
      if (current->UniqueOne()) {
        return true;
      }
      current = current->next_;
      if (current == this) {
        return false;
      }
    }
  }

  bool UniqueOne() const { return buf_.unique(); }

  ConstIterator cbegin() const;
  ConstIterator cend() const;

  Iterator begin();
  Iterator end();

  ConstIterator begin() const;
  ConstIterator end() const;

  class DataPointer {
   public:
    explicit DataPointer(const IOBuf* p) : start_(p), p_(p) {}

    const uint8_t* Data() {
      if (!p_ || p_->Length() - offset_ == 0)
        throw std::runtime_error("DataPointer::Data() past end of buffer");

      return p_->Data() + offset_;
    }

    const uint8_t* GetNoAdvance(size_t len) {
      if (!p_)
        throw std::runtime_error("DataPointer::Get(): past end of buffer");

      if (p_->Length() - offset_ < len) {
        // request straddles buffers, allocate a new chunk of memory to copy it
        // into (so it is contiguous)
        chunk_list_.emplace_front();
        auto& chunk = chunk_list_.front();
        chunk.reserve(len);
        auto p = p_;
        auto tmp_len = len;
        auto offset = offset_;
        while (tmp_len > 0 && p) {
          auto remainder = p->Length() - offset;
          auto data = p->Data() + offset;
          chunk.insert(chunk.end(), data, data + remainder);
          p = p->Next();
          if (p == start_)
            p = nullptr;
          offset = 0;
          tmp_len -= remainder;
        }

        if (!p && tmp_len > 0)
          throw std::runtime_error("DataPointer::Get(): past end of buffer");

        return chunk.data();
      }

      return p_->Data() + offset_;
    }

    template <typename T> const T& GetNoAdvance() {
      return *reinterpret_cast<const T*>(GetNoAdvance(sizeof(T)));
    }

    const uint8_t* Get(size_t len) {
      auto ret = GetNoAdvance(len);
      Advance(len);
      return ret;
    }

    template <typename T> const T& Get() {
      const auto& ret = GetNoAdvance<T>();
      Advance(sizeof(T));
      return ret;
    }

    void Advance(size_t size) {
      while (p_) {
        auto remainder = p_->Length() - offset_;
        if (remainder >= size) {
          offset_ += size;
          return;
        } else {
          p_ = p_->Next();
          if (p_ == start_)
            p_ = nullptr;
          offset_ = 0;
          size -= remainder;
        }
      }
#ifdef __ebbrt__
      kabort("Advance fail\n");
#endif
      throw std::runtime_error("DataPointer::Advance(): past end of buffer");
    }

   private:
    const IOBuf* start_{nullptr};
    const IOBuf* p_{nullptr};
    size_t offset_{0};
    std::forward_list<std::vector<uint8_t>> chunk_list_;
  };

  DataPointer GetDataPointer() const { return DataPointer(this); }

 private:
  IOBuf(const IOBuf&) = default;
  IOBuf& operator=(const IOBuf&) = default;
  IOBuf* next_{this};
  IOBuf* prev_{this};
  uint8_t* data_{nullptr};
  std::shared_ptr<uint8_t> buf_{nullptr};
  size_t length_{0};
  size_t capacity_{0};
};

class IOBuf::Iterator
    : public boost::iterator_facade<IOBuf::Iterator, IOBuf,
                                    boost::forward_traversal_tag> {
 public:
  Iterator(IOBuf* pos, IOBuf* end) : pos_(pos), end_(end) {}

 private:
  IOBuf& dereference() const { return *pos_; }

  bool equal(const Iterator& other) const {
    return pos_ == other.pos_ && end_ == other.end_;
  }

  void increment() {
    pos_ = pos_->Next();

    if (pos_ == end_)
      pos_ = end_ = nullptr;
  }

  IOBuf* pos_;
  IOBuf* end_;

  friend class boost::iterator_core_access;
};

class IOBuf::ConstIterator
    : public boost::iterator_facade<IOBuf::ConstIterator, const IOBuf,
                                    boost::forward_traversal_tag> {
 public:
  ConstIterator(const IOBuf* pos, const IOBuf* end) : pos_(pos), end_(end) {}

 private:
  const IOBuf& dereference() const { return *pos_; }

  bool equal(const ConstIterator& other) const {
    return pos_ == other.pos_ && end_ == other.end_;
  }

  void increment() {
    pos_ = pos_->Next();

    if (pos_ == end_)
      pos_ = end_ = nullptr;
  }

  const IOBuf* pos_;
  const IOBuf* end_;

  friend class boost::iterator_core_access;
};

inline IOBuf::ConstIterator IOBuf::begin() const { return cbegin(); }
inline IOBuf::ConstIterator IOBuf::end() const { return cend(); }
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_BUFFER_H_
