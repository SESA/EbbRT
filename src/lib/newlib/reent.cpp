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
#include <sys/reent.h>

namespace {
  bool init = false;
  struct _reent reent;
}

extern "C" void ebbrt_init_reent(struct _reent *reent);

extern "C" struct _reent *
ebbrt_newlib_getreent(void)
{
  if(!init) {
    ebbrt_init_reent(&reent);
    init = true;
  }
  return &reent;
}
