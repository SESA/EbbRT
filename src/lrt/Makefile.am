# EbbRT: Distributed, Elastic, Runtime
# Copyright (C) 2013 SESA Group, Boston University

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.

# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
include src/lrt/arch/Makefile.am
include src/lrt/bare/Makefile.am
include src/lrt/ulnx/Makefile.am

lrt_sources = \
	$(lrt_arch_sources) \
	src/lrt/config.hpp \
	src/lrt/event.cpp \
	src/lrt/event.hpp \
	src/lrt/event_impl.hpp \
	src/lrt/trans.cpp \
	src/lrt/trans.hpp \
	src/lrt/trans_impl.hpp

if LRT_ULNX
lrt_sources += $(lrt_ulnx_sources)
else
if LRT_BARE
lrt_sources += $(lrt_bare_sources)
endif
endif
