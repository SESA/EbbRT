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
#include <unordered_map>
#include <fstream>
#include "app.hpp"

namespace ebbrt {
  namespace app {
    // Only for use after dynamic memory allocation
    std::unordered_map<std::string, ConfigFuncPtr> map_ __attribute__((init_priority(101)));
  
    void AddSymbol (std::string str, ConfigFuncPtr func) {
      map_[str] = func;
    };
    ConfigFuncPtr LookupSymbol (std::string str) {
      auto it =  map_.find(str);
      if (it != map_.end()) {
	//it->second(data, header->caplen);
	return it->second;
      }
      return nullptr;
    }

    char* LoadFile(char* path, int* len)
    {
      std::ifstream is (path, std::ifstream::binary);
      *len=0;
      if (is) {
        is.seekg (0, is.end);
        *len = is.tellg();
        is.seekg (0, is.beg);

        auto buf = new char[*len+1024]; //padding for fun
        is.read (buf,*len);
        is.close();
        return buf;
      }
      return nullptr;
    }

    void SaveFile(char* ptr, int len, char *filepath)
    {
      std::ofstream outfile (filepath, std::ofstream::binary);
      outfile.write (ptr,len);
      outfile.close();
    }
  }
}
