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
#ifndef EBBRT_EBB_STREAM_STREAM_HPP
#define EBBRT_EBB_STREAM_STREAM_HPP

#include <functional>

#include "ebb/ebb.hpp"

namespace ebbrt {
  class StreamProcessor : public EbbRep {
  public:
    virtual void Process(const char*, size_t) = 0;
  };

  class Stream : public EbbRep {
  public:
    enum Split {
      Duplicate,
      Divide
    };
    virtual void Attach(std::function<void(const char*, size_t)> processor,
                        Split how_to_split) = 0;
    virtual void Attach(EbbRef<StreamProcessor> processor,
                        Split how_to_split) = 0;
  };
}

#endif
