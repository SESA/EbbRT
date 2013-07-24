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

#include "app/app.hpp"
#include "ebb/EbbManager/EbbManager.hpp"
#include "src/lrt/bare/config.hpp"

namespace ebbrt {
  namespace lrt {

void dump_config_info() 
{
  struct ebbrt::lrt::SymTabEntry *stptr = ebbsymtab_start;
  // dump symbol table, move this elsewhere
  lrt::console::write("Dumping symbol table\n");
  while (stptr < ebbsymtab_end) {
    lrt::console::write("\t");
    lrt::console::write(stptr->str);
    lrt::console::write("\n");
    stptr++;
  }
}


app::ConfigFuncPtr 
LookupSymbol(const char *str)
{
  // dump_config_info();
  // FIXME
  struct ebbrt::lrt::SymTabEntry *stptr = ebbsymtab_start;
  while (stptr < ebbsymtab_end) {
    if (strcmp(str,(stptr->str)) == 0) {
      return stptr->config_func;
    } else {
#if 0
      lrt::console::write("lookup: ");
      lrt::console::write(stptr->str);
      lrt::console::write(" != ");
      lrt::console::write(str);
      lrt::console::write("\n");
#endif
    }
    stptr++;
  }
  return nullptr;
}
  }
}
