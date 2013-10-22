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

#include <stdio.h>
#include <cassert>
#include <sstream>
#include "ebb/SharedRoot.hpp"
#include "ebb/Config/Fdt.hpp"
#include "lib/fdt/libfdt.h"


ebbrt::EbbRoot*
ebbrt::Fdt::ConstructRoot()
{
  return new SharedRoot<Fdt>();
}

ebbrt::Fdt::Fdt(EbbId id) : Config{id} {} 

void
ebbrt::Fdt::SetConfig(void *config)
{
  assert(!fdt_check_header(config));
  initialized_ = true;
  fdt_ = config;
}

void*
ebbrt::Fdt::GetConfig()
{
  return fdt_;
}

uint32_t
ebbrt::Fdt::GetInt32(std::string path, std::string prop) 
{
  int len;
  int offset =  fdt_path_offset(fdt_, path.c_str());
  const unsigned char *id = (const unsigned char *)fdt_getprop(fdt_, offset, prop.c_str(), &len);

  assert(len == 4);
  // machine independant byte ordering of litte-endian value
  return ((id[3]<<0)|(id[2]<<8)|(id[1]<<16)|(id[0]<<24));
}

uint64_t
ebbrt::Fdt::GetInt64(std::string path, std::string prop) 
{
  int len;
  int offset =  fdt_path_offset(fdt_, path.c_str());
  const uint64_t *id = (const uint64_t *)fdt_getprop(fdt_, offset, prop.c_str(), &len);

  assert(len == 8);
  // machine independant byte ordering of litte-endian value
  return ((id[7]<<0)|(id[6]<<8)|(id[5]<<16)|(id[4]<<24)|(id[3]<<32)|(id[2]<<40)|(id[1]<<48)|(id[0]<<56));
}

std::string
ebbrt::Fdt::GetString(std::string path, std::string prop) 
{
  int len;
  std::string ret;
  int offset =  fdt_path_offset(fdt_, path.c_str());
  const unsigned char *id = (const unsigned char *)fdt_getprop(fdt_, offset, prop.c_str(), &len);
  assert(len > 0);
  ret.append((const char*)id, len);
  return ret;
}

void 
ebbrt::Fdt::SetInt32(std::string path, std::string prop, uint32_t val) 
{
  int offset =  fdt_path_offset(fdt_, path.c_str());
  fdt_setprop_u32(fdt_, offset, prop.c_str(), val);
}
void 
ebbrt::Fdt::SetInt64(std::string path, std::string prop, uint64_t val) 
{
  int offset =  fdt_path_offset(fdt_, path.c_str());
  fdt_setprop_u64(fdt_, offset, prop.c_str(), val);
}

void 
ebbrt::Fdt::SetString(std::string path, std::string prop, std::string val) 
{
  int offset =  fdt_path_offset(fdt_, path.c_str());
//fdt_setprop_string(fdt_, offset, prop.c_str(), val.c_str());
	fdt_setprop(fdt_, offset, prop.c_str(), val.c_str(), val.length()+1);
}
