/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef EBBRT_MISC_BUFFER_HPP
#define EBBRT_MISC_BUFFER_HPP

#include <cstdlib>

#include <list>
#include <memory>

namespace ebbrt {
  typedef std::list<std::pair<const void*, size_t> > BufferList;

  class Buffer {
    std::shared_ptr<char> ptr_;
    size_t length_;
  public:
    Buffer(void* buffer, size_t length)
      : ptr_{static_cast<char*>(buffer), std::free},
      length_{length}
    {
    }

    template <typename Deleter>
    Buffer(void* buffer, size_t length, Deleter deleter)
      : ptr_{static_cast<char*>(buffer), deleter},
      length_{length}
    {
    }

    Buffer operator+(size_t offset) {
      auto buf = Buffer{ptr_.get() + offset, length_ - offset,
                        ptr_};
      ptr_.reset();
      return std::move(buf);
    }

    Buffer operator-(size_t offset) {
      auto buf = Buffer{ptr_.get() - offset, length_ + offset,
                        ptr_};
      ptr_.reset();
      return std::move(buf);
    }

    size_t length() const
    {
      return length_;
    }

    char& operator[](size_t pos) const
    {
      return ptr_.get()[pos];
    }

    char* data() const
    {
      return ptr_.get();
    }
  private:
    Buffer(char* buffer, size_t length,
           const std::shared_ptr<char>& shared_ptr)
      : ptr_{shared_ptr, buffer}, length_{length}
    {
    }
  };


}

#endif
