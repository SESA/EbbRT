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
#ifndef LRT_BARE_CONFIG_HPP
#define LRT_BARE_CONFIG_HPP

#include "app/app.hpp"

namespace ebbrt {
  namespace lrt {
    
    struct SymTabEntry {
      /* 
	 FIXME:function here should take an argument to section of
	 flattened device tree, and should return void
      */
      ebbrt::EbbRoot* (*config_func)(); 
      char *str;
    };

    void dump_config_info();
    /// lookup return function poitner
    app::ConfigFuncPtr LookupSymbol(const char *str);
  }
}


/* 
 * Symbol table for functions needed early in boot.
 */
extern struct ebbrt::lrt::SymTabEntry ebbsymtab_start[];
extern struct ebbrt::lrt::SymTabEntry ebbsymtab_end[];

#define ADD_EARLY_CONFIG_SYMBOL(symbol,func)		\
char __table ## symbol[] = #symbol;					\
							\
ebbrt::lrt::SymTabEntry					\
__table ## symbol ## _entry __attribute__((section("ebbsymtab")))	\
= {							\
  .config_func = func,					\
  .str = __table ## symbol					\
};

#endif /*LRT_BARE_CONFIG_HPP*/

