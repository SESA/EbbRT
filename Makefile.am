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

ACLOCAL_AMFLAGS = -I m4

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

AM_CXXFLAGS = -std=c++11 -Wall -Werror
AM_CPPFLAGS = -iquote $(top_srcdir)/src \
	-iquote $(top_srcdir)/src/ebb/Network/lwip-1.4.1/src/include \
	-iquote $(top_srcdir)/src/ebb/Network/lwip-1.4.1/src/include/ipv4 \
	-iquote $(top_srcdir)/src/ebb/Network \
	--include $(top_builddir)/config.h

#lib_LIBRARIES = libebbrt.a
lib_LTLIBRARIES = libebbrt.la

libebbrt_la_SOURCES = \
	$(arch_sources) \
	$(app_sources) \
	$(device_sources) \
	$(ebb_sources) \
	$(lrt_sources) \
	$(misc_sources)
libebbrt_la_LIBADD =
libebbrt_la_DEPENDENCIES =
EXTRA_DIST =
EXTRA_LTLIBRARIES =
EXTRA_LIBRARIES =
CLEANFILES =
ebbrt_libs =
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
COMMA := ,
if LRT_BARE
STATIC_LIBRARY_PREFIX := lib
else
STATIC_LIBRARY_PREFIX :=
endif
app_linkflags =
app_deps =

%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=$(dir $^) --cpp_out=$(dir $^) $^


%.dtb: %.dts
	dtc -O dtb -o $@ $<

if LRT_BARE

.PRECIOUS: %.dtb %.elf32

%.elf32: %
	objcopy -O elf32-i386 $< $@

if ARCH_X86_64
app_linkflags += -Wl,-n,-z,max-page-size=0x1000 \
	-Wl,-T,$(top_srcdir)/src/lrt/bare/arch/x86_64/link.ld

%.iso: % %.dtb %.elf32 util/grub.cfg
	strip -s $< -o $<.stripped
	grub-mkrescue -o $@ -graft-points boot/ebbrt=$<.stripped \
	boot/fdt=$<.dtb \
	boot/grub/grub.cfg=$(top_srcdir)/util/grub.cfg
endif

#end of LRT_BARE
endif

all_apps: $(EXTRA_PROGRAMS)

include docs/Makefile.am
include src/arch/Makefile.am
include src/app/Makefile.am
include src/ebb/Makefile.am
include src/lib/Makefile.am
include src/lrt/Makefile.am
include test/Makefile.am
