//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_IOBUF_H_
#define COMMON_SRC_INCLUDE_EBBRT_IOBUF_H_

#include <algorithm>
#include <cassert>
#include <cstring>
#include <forward_list>
#include <memory>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>

namespace ebbrt {
class IOBuf {
 public:
  class ConstIterator;
  class Iterator;

  IOBuf() noexcept;
  IOBuf(const uint8_t* data, size_t length) noexcept;

  virtual ~IOBuf() noexcept;

  template <typename T, typename... Args>
  static std::unique_ptr<T> Create(Args&&... args) {
    auto ptr = new T(std::forward<Args>(args)...);
    return std::unique_ptr<T>(ptr);
  }

  const uint8_t* Data() const { return data_; }

  const uint8_t* Tail() const { return Data() + Length(); }

  size_t Length() const { return length_; }

  virtual size_t Capacity() const = 0;

  virtual const uint8_t* Buffer() const = 0;

  const uint8_t* BufferEnd() const { return Buffer() + Capacity(); }

  IOBuf* Next() { return next_; }

  const IOBuf* Next() const { return next_; }

  IOBuf* Prev() { return prev_; }

  const IOBuf* Prev() const { return prev_; }

  void Advance(size_t amount) {
    assert(length_ >= amount);
    data_ += amount;
    length_ -= amount;
  }

  void Retreat(size_t amount) {
    assert((data_ - amount) >= Buffer());
    data_ -= amount;
    length_ += amount;
  }

  void TrimEnd(size_t amount) { length_ -= amount; }
  void SetLength(size_t amount) { length_ = amount; }

  bool IsChained() const { return Next() != this; }

  size_t CountChainElements() const;

  size_t ComputeChainDataLength() const;

  void AdvanceChain(size_t amount);

  void PrependChain(std::unique_ptr<IOBuf> iobuf);
  void AppendChain(std::unique_ptr<IOBuf> iobuf) {
    Next()->PrependChain(std::move(iobuf));
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

  // Remove end and the rest of the chain from the chain that this is the head
  // of. Returns end as a unique_ptr
  std::unique_ptr<IOBuf> UnlinkEnd(IOBuf& end) {
    auto new_end = end.prev_;
    end.prev_ = prev_;
    // if unlinking end of chain
    if (end.next_ == this)
      end.next_ = &end;
    prev_->next_ = &end;
    prev_ = new_end;
    new_end->next_ = this;

    return std::unique_ptr<IOBuf>(&end);
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

  ConstIterator cbegin() const;
  ConstIterator cend() const;

  Iterator begin();
  Iterator end();

  ConstIterator begin() const;
  ConstIterator end() const;

  class DataPointer {
   public:
    explicit DataPointer(const IOBuf* p) : p_{p} {
      assert(p->ComputeChainDataLength() > 0);
      // Iterate past any empty buffers up front
      while (p_->Length() == 0) {
        p_ = p_->Next();
      }
    }

    const uint8_t* Data() {
      assert(p_->Length() > 0);
      if (p_->Length() - offset_ == 0)
        throw std::runtime_error("DataPointer::Data() past end of buffer");

      return p_->Data() + offset_;
    }

    const uint8_t* GetNoAdvance(size_t size) {
      if (!p_)
        throw std::runtime_error("DataPointer::Get(): past end of buffer");

      assert(p_->Length() > 0);
      if (p_->Length() - offset_ < size) {
        // request straddles buffers, allocate a new chunk of memory to copy it
        // into (so it is contiguous)
        chunk_list.emplace_front();
        auto& chunk = chunk_list.front();
        chunk.reserve(size);
        auto p = p_;
        auto len = size;
        auto offset = offset_;
        while (len > 0) {
          auto remainder = std::min(p->Length() - offset, len);
          auto data = p->Data() + offset;
          chunk.insert(chunk.end(), data, data + remainder);
          p = p->Next();
          offset = 0;
          len -= remainder;
        }

        return reinterpret_cast<const uint8_t*>(chunk.data());
      }

      return Data();
    }

    // copies 'size' bytes into byte pointer 'buf'
    void GetNoAdvance(size_t size, uint8_t* buf) {
      if (!p_)
        throw std::runtime_error("DataPointer::Get(): past end of buffer");

      assert(p_->Length() > 0);

      // saves buf pointer
      uint8_t* tmptr = buf;

      if (p_->Length() - offset_ < size) {
        auto p = p_;
        auto len = size;
        auto offset = offset_;
        while (len > 0) {
          // get number of bytes left
          auto remainder = std::min(p->Length() - offset, len);
          // get offset into where to start copy from
          auto data = p->Data() + offset;
          std::memcpy(tmptr, data, remainder);
          tmptr += remainder;
          // go to next chain
          p = p->Next();
          offset = 0;
          len -= remainder;
        }
      } else {  // no need to memcpy from chains
        std::memcpy(tmptr, Data(), size);
      }
    }
    template <typename T> const T& GetNoAdvance() {
      return *reinterpret_cast<const T*>(GetNoAdvance(sizeof(T)));
    }

    const uint8_t* Get(size_t size) {
      auto ret = GetNoAdvance(size);
      Advance(size);
      return ret;
    }

    // copies 'size' bytes into byte pointer 'buf'
    void Get(size_t size, uint8_t* buf) {
      GetNoAdvance(size, buf);
      Advance(size);
    }

    template <typename T> const T& Get() {
      return *reinterpret_cast<const T*>(Get(sizeof(T)));
    }

    void Advance(size_t size) {
      assert(p_->Length() > 0);
      while (size > 0) {
        auto remainder = p_->Length() - offset_;
        if (remainder > size) {
          offset_ += size;
          return;
        }
        p_ = p_->Next();
        offset_ = 0;
        size -= remainder;
      }

      while (p_->Length() == 0) {
        p_ = p_->Next();
      }
    }

   private:
    const IOBuf* p_{nullptr};
    size_t offset_{0};
    std::forward_list<std::vector<char>> chunk_list;
  };

  DataPointer GetDataPointer() const { return DataPointer(this); }

 protected:
  void SetView(const IOBuf& other) {
    assert(Buffer() == other.Buffer());
    assert(Capacity() == other.Capacity());
    data_ = other.Data();
    length_ = other.Length();
  }

 private:
  IOBuf* next_{this};
  IOBuf* prev_{this};
  const uint8_t* data_{nullptr};
  size_t length_{0};
};

class MutIOBuf : public IOBuf {
 public:
  class ConstIterator;
  class Iterator;

  MutIOBuf() noexcept;
  MutIOBuf(const uint8_t* data, size_t length) noexcept;
  virtual ~MutIOBuf() {}

  uint8_t* MutData() { return const_cast<uint8_t*>(Data()); }

  uint8_t* MutTail() { return const_cast<uint8_t*>(Tail()); }

  uint8_t* MutBuffer() { return const_cast<uint8_t*>(Buffer()); }

  uint8_t* MutBufferEnd() { return const_cast<uint8_t*>(BufferEnd()); }

  MutIOBuf* Next() { return static_cast<MutIOBuf*>(IOBuf::Next()); }

  const MutIOBuf* Next() const {
    return static_cast<const MutIOBuf*>(IOBuf::Next());
  }

  MutIOBuf* Prev() { return static_cast<MutIOBuf*>(IOBuf::Prev()); }

  const MutIOBuf* Prev() const {
    return static_cast<const MutIOBuf*>(IOBuf::Prev());
  }

  ConstIterator cbegin() const;
  ConstIterator cend() const;

  Iterator begin();
  Iterator end();

  ConstIterator begin() const;
  ConstIterator end() const;

  class MutDataPointer {
   public:
    explicit MutDataPointer(MutIOBuf* p) : p_{p} {
      assert(p->ComputeChainDataLength() > 0);
      // Iterate past any empty buffers up front
      while (p_->Length() == 0) {
        p_ = p_->Next();
      }
    }

    uint8_t* Data() {
      assert(p_->Length() > 0);
      if (p_->Length() - offset_ == 0)
        throw std::runtime_error("MutDataPointer::Data() past end of buffer");

      return p_->MutData() + offset_;
    }

    template <typename T> T& GetNoAdvance() {
      assert(p_->Length() > 0);
      if (p_->Length() - offset_ < sizeof(T)) {
        throw std::runtime_error(
            "MutDataPointer::Get(): request straddles buffer");
      }

      return *reinterpret_cast<T*>(Data());
    }

    template <typename T> T& Get() {
      auto& ret = GetNoAdvance<T>();
      Advance(sizeof(T));
      return ret;
    }

    void Advance(size_t size) {
      assert(p_->Length() > 0);
      while (size > 0) {
        auto remainder = p_->Length() - offset_;
        if (remainder > size) {
          offset_ += size;
          return;
        }
        p_ = p_->Next();
        offset_ = 0;
        size -= remainder;
      }

      while (p_->Length() == 0) {
        p_ = p_->Next();
      }
    }

   private:
    MutIOBuf* p_{nullptr};
    size_t offset_{0};
  };

  MutDataPointer GetMutDataPointer() { return MutDataPointer(this); }
};

template <typename T> class IOBufBase : public T, public IOBuf {
 public:
  template <typename... Args>
  explicit IOBufBase(Args&&... args)
      : T(std::forward<Args>(args)...), IOBuf(T::Buffer(), T::Capacity()) {}

  const uint8_t* Buffer() const override { return T::Buffer(); }

  size_t Capacity() const override { return T::Capacity(); }
};

template <typename T> class MutIOBufBase : public T, public MutIOBuf {
 public:
  template <typename... Args>
  explicit MutIOBufBase(Args&&... args)
      : T(std::forward<Args>(args)...), MutIOBuf(T::Buffer(), T::Capacity()) {}

  const uint8_t* Buffer() const override { return T::Buffer(); }

  size_t Capacity() const override { return T::Capacity(); }
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

class MutIOBuf::Iterator
    : public boost::iterator_facade<MutIOBuf::Iterator, MutIOBuf,
                                    boost::forward_traversal_tag> {
 public:
  Iterator(MutIOBuf* pos, MutIOBuf* end) : pos_(pos), end_(end) {}

 private:
  MutIOBuf& dereference() const { return *pos_; }

  bool equal(const Iterator& other) const {
    return pos_ == other.pos_ && end_ == other.end_;
  }

  void increment() {
    pos_ = pos_->Next();

    if (pos_ == end_)
      pos_ = end_ = nullptr;
  }

  MutIOBuf* pos_;
  MutIOBuf* end_;

  friend class boost::iterator_core_access;
};

class MutIOBuf::ConstIterator
    : public boost::iterator_facade<MutIOBuf::ConstIterator, const MutIOBuf,
                                    boost::forward_traversal_tag> {
 public:
  ConstIterator(const MutIOBuf* pos, const MutIOBuf* end)
      : pos_(pos), end_(end) {}

 private:
  const MutIOBuf& dereference() const { return *pos_; }

  bool equal(const ConstIterator& other) const {
    return pos_ == other.pos_ && end_ == other.end_;
  }

  void increment() {
    pos_ = pos_->Next();

    if (pos_ == end_)
      pos_ = end_ = nullptr;
  }

  const MutIOBuf* pos_;
  const MutIOBuf* end_;

  friend class boost::iterator_core_access;
};

inline MutIOBuf::ConstIterator MutIOBuf::begin() const { return cbegin(); }
inline MutIOBuf::ConstIterator MutIOBuf::end() const { return cend(); }
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_IOBUF_H_
