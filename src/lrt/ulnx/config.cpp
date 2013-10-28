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
#include <cassert>
#include "ebbrt.hpp"
#include "app/app.hpp"
#include "lib/fdt/libfdt.h"
#include "lrt/EbbId.hpp"
#include "lrt/config.hpp"
#include "lrt/ulnx/init.hpp"

namespace ebbrt {
  namespace lrt {
    namespace config {

      uint32_t
        fdt_getint32(void* fdt, int root, const char *prop)
        {
          if(!fdt)
            assert(0);

          int len;
          const unsigned char *id = (const unsigned char *)fdt_getprop(fdt, root, prop, &len);
          assert(len == 4);
          // machine independant byte ordering of litte-endian value
          return ((id[3]<<0)|(id[2]<<8)|(id[1]<<16)|(id[0]<<24));
        }

      trans::EbbId
        find_static_ebb_id(void* fdt, const char* name)
        {
          if(fdt)
          {
            std::string path="/ebbs/";
            path.append(name);
            int offset =  fdt_path_offset(fdt, path.c_str());
            return fdt_getint32(fdt, offset, "id");

          }else
          {
            // temporary configuration kludge
            for (unsigned i = 0; i < app::config.num_statics; ++i) {
              if (std::strcmp(app::config.static_ebb_ids[i].name, name) == 0) {
                return app::config.static_ebb_ids[i].id;
              }
            }
          }
          assert(0); // assert if lookup failed
        }

      uint16_t
        get_space_id(void* fdt)
        {
          if(fdt)
            return fdt_getint32(fdt, 0, "space_id");
          else
            return 0; // temporary configuration kludge
        }

      uint32_t
        get_static_ebb_count(void* fdt)
        {
          if(fdt)
            return fdt_getint32(fdt, 0, "linux_num_static_init");
          else
            return app::config.num_statics; // temporary configuration kludge
        }

      bool
        get_multicore(void* fdt)
        {
          if(fdt)
            return fdt_getint32(fdt, 0, "multicore");
          else
            return 0; // temporary configuration kludge
        }
    }
  }
}
