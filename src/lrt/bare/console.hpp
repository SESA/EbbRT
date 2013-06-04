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
#ifndef LRT_BARE_CONSOLE_HPP
#define LRT_BARE_CONSOLE_HPP

namespace ebbrt {
  namespace lrt {
    /** The early boot console to be used from within the lrt */
    namespace console {
      /** Initialize the console */
      void init();
      /**
       * Write a character to the console
       * @param[in] c The character to write
       */
      void write(char c);
      /**
       * Write a specified amount of data to the console
       * @param[in] str The data to write
       * @param[in] len The amount of data to write
       */
      int write(const char *str, int len);
      /**
       * Write a null terminated string to the console
       * @param[in] str The null terminated string
       */
      void write(const char *str);
    }
  }
}

#endif
