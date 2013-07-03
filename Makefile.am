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

if HAVE_PYTHON
TESTS_ENVIRONMENT = python
if LRT_ULNX
check_SCRIPTS = test/helloworld.py
TESTS = test/helloworld.py
endif
if LRT_BARE
check_SCRIPTS = test/hello-bare.py
TESTS = test/hello-bare.py
endif
endif

AM_CXXFLAGS = -std=c++11 -Wall -Werror -Wno-unused-private-field
AM_CPPFLAGS = -iquote $(top_srcdir)/src \
	--include $(top_builddir)/config.h

lib_LIBRARIES = libebbrt.a

libebbrt_a_SOURCES = \
	$(arch_sources) \
	$(app_sources) \
	$(device_sources) \
	$(ebb_sources) \
	$(lrt_sources) \
	$(misc_sources)
libebbrt_a_LIBADD =
libebbrt_a_DEPENDENCIES =
EXTRA_DIST =
EXTRA_LIBRARIES =
ebbrt_libs =

app_linkflags =
app_deps =

if LRT_BARE

if ARCH_X86_64
app_linkflags += -Wl,-n,-z,max-page-size=0x1000 \
	-Wl,-T,$(top_srcdir)/src/lrt/bare/arch/x86_64/link.ld

%.iso: % util/grub.cfg
	strip -s $< -o $<.stripped
	grub-mkrescue -o $@ -graft-points boot/ebbrt=$<.stripped \
	boot/grub/grub.cfg=$(top_srcdir)/util/grub.cfg
endif

if ARCH_POWERPC
#powerpc build specifics
endif

#end of LRT_BARE
endif

include docs/Makefile.am
include src/arch/Makefile.am
include src/app/Makefile.am
include src/ebb/Makefile.am
include src/lib/Makefile.am
include src/lrt/Makefile.am
include test/Makefile.am
