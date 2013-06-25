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
#ifndef EBBRT_EBB_WORKBAG_WORKBAG_HPP
#define EBBRT_EBB_WORKBAG_WORKBAG_HPP

#include <forward_list>
#include <functional>

#ifdef __linux__
#include <mutex>
#endif

#include "ebb/ebb.hpp"

namespace ebbrt {
  class WorkBag : public EbbRep {
  public:
    static EbbRoot* ConstructRoot();

    typedef std::function<void(const char*, size_t)> callback;
    virtual void Get(callback cb);
    virtual void Add(char* data, size_t size);
    virtual void HandleMessage(NetworkId from,
                               const char* buf,
                               size_t len) override;
  private:
#ifdef __linux__
    std::forward_list<std::pair<char*, size_t> > bag_;
    std::mutex lock_;
#elif __ebbrt__
    std::forward_list<callback> cbs_;
    Spinlock lock_;
#endif
  };
  const EbbRef<WorkBag> workbag =
    EbbRef<WorkBag>(lrt::trans::find_static_ebb_id("WorkBag"));
}

#endif
