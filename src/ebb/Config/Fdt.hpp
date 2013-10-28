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
#ifndef EBBRT_EBB_ETHERNET_RAWSOCKET_HPP
#define EBBRT_EBB_ETHERNET_RAWSOCKET_HPP

#include "ebb/Config/Config.hpp"

namespace ebbrt {
  class Fdt : public Config {
  public:
    static EbbRoot* ConstructRoot();
    Fdt(EbbId id);
    void SetConfig(void *config) override;
    void* GetConfig() override;

    uint32_t GetInt32(std::string path, std::string prop) override;
    uint64_t GetInt64(std::string path, std::string prop) override;
    std::string GetString(std::string path, std::string prop) override;

    void SetInt32(std::string path, std::string prop, uint32_t val) override;
    void SetInt64(std::string path, std::string prop, uint64_t val) override;
    void SetString(std::string path, std::string prop, std::string val) override;
  private:
    void *fdt_;
    bool initialized_;
  };
}

#endif
