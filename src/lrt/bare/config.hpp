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
#ifndef EBBRT_LRT_CONFIG_HPP
#error "Don't include this file directly"
#endif

#include "app/app.hpp"

namespace ebbrt {
  namespace lrt {
    namespace config {

      struct SymTabEntry {
        /* 
          FIXME:function here should take an argument to section of
          flattened device tree, and should return void
        */
        trans::EbbRoot* (*config_func)(); 
        char *str;
      };

      /**
       * @brief Look up reference in the pre-init symbol table
       * Early init ebbs must add a reference to thier root 
       * constructor to the symbol table using the 
       * ADD_EARLY_CONFIG_SYMBOL macro. This allows the bootstapping 
       * early ebb constructors before general C++ constructors. 
       *
       * @param str Ebb name
       *
       * @return Pointer to corresponding function's symbol 
       */
      app::ConfigFuncPtr LookupSymbol(const char *str);



      /**
       * @brief Get byte-ordered uint32 from fdt 
       *
       * @param root fdt offset
       * @param prop property name
       *
       * @return 
       */
      uint32_t fdt_getint32(int root, const char *prop);
    }
  }
}


/* 
 * Symbol table for functions needed early in boot.
 */
extern struct ebbrt::lrt::config::SymTabEntry ebbsymtab_start[];
extern struct ebbrt::lrt::config::SymTabEntry ebbsymtab_end[];

#define ADD_EARLY_CONFIG_SYMBOL(symbol,func)		\
  char __table ## symbol[] = #symbol;					\
\
ebbrt::lrt::config::SymTabEntry					\
__table ## symbol ## _entry __attribute__((section("ebbsymtab")))	\
= {							\
  .config_func = func,					\
  .str = __table ## symbol					\
};


