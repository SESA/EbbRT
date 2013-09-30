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

#include <string>

#include "app/app.hpp"
#include "ebb/EbbManager/EbbManager.hpp"
#include "lib/fdt/libfdt.h"
#include "src/lrt/bare/assert.hpp"
#include "src/lrt/bare/boot.hpp"
#include "src/lrt/config.hpp"

namespace ebbrt {
  namespace lrt {
    namespace config {

      app::ConfigFuncPtr 
        LookupSymbol(const char *str)
        {
          struct SymTabEntry *stptr = ebbsymtab_start;
          while (stptr < ebbsymtab_end) {
            if (strcmp(str,(stptr->str)) == 0) {
              return stptr->config_func;
            } else {
            }
            stptr++;
          }
          return nullptr;
        }


      uint32_t
        fdt_getint32(int root, const char *prop)
        {
          int len;
          const unsigned char *id = (const unsigned char *)fdt_getprop(ebbrt::lrt::boot::fdt, root, prop, &len);
          LRT_ASSERT(len == 4);
          // machine independant byte ordering of litte-endian value
          return ((id[3]<<0)|(id[2]<<8)|(id[1]<<16)|(id[0]<<24));
        }

      uint32_t
        find_static_ebb_id(const char* name)
        {
          std::string path="/ebbs/";
          path.append(name);
          int offset =  fdt_path_offset(ebbrt::lrt::boot::fdt, path.c_str());
          return fdt_getint32(offset, "id");
        }

      uint16_t
        get_space_id(void)
        {
          return (uint16_t)(fdt_getint32(0, "space_id") & 0xFFFF);
        }

      uint32_t
        get_static_ebb_count(void)
        {
          return fdt_getint32(0, "ebbrt_num_static_init");
        }

      bool
        get_multicore(void)
        {
          return (bool)fdt_getint32(0, "multicore");
        }
    }
  }
}
