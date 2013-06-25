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
lrt_bare_arch_x86_64_sources = \
	src/lrt/bare/arch/x86_64/boot.cpp \
	src/lrt/bare/arch/x86_64/boot.hpp \
	src/lrt/bare/arch/x86_64/bootasm.S \
	src/lrt/bare/arch/x86_64/event.cpp \
	src/lrt/bare/arch/x86_64/event.hpp \
	src/lrt/bare/arch/x86_64/isr.S \
	src/lrt/bare/arch/x86_64/mem.cpp \
	src/lrt/bare/arch/x86_64/smp.S \
	src/lrt/bare/arch/x86_64/trans.cpp

if ARCH_X86_64
app_deps += src/lrt/bare/arch/x86_64/link.ld
endif

EXTRA_DIST += src/lrt/bare/arch/x86_64/link.ld
