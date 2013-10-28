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
#ifndef EBBRT_EBB_CONFIG_CONFIG_HPP
#define EBBRT_EBB_CONFIG_CONFIG_HPP

#include <functional>
#include "ebb/ebb.hpp"

namespace ebbrt {
  class Config : public EbbRep {
  public:
    virtual void 
      SetConfig(void *config) = 0;
    virtual void *
      GetConfig() = 0;
    virtual uint32_t
      GetInt32(std::string path, std::string prop) = 0;
    virtual uint64_t
      GetInt64(std::string path, std::string prop) = 0;
    virtual std::string
      GetString(std::string path, std::string prop) = 0;
    virtual void
      SetInt32(std::string path, std::string prop, uint32_t val) = 0;
    virtual void
      SetInt64(std::string path, std::string prop, uint64_t val) = 0;
    virtual void
      SetString(std::string path, std::string prop, std::string val) = 0;
  protected:
    Config(EbbId id) : EbbRep{id} {}
  };
  extern EbbRef<Config> config_handle;
}

#endif
