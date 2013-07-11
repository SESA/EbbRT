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
EXTRA_PROGRAMS += \
	src/app/LibFox/mcphoton \
	src/app/LibFox/tfox

src_app_LibFox_mcphoton_SOURCES = \
	src/app/LibFox/app.cpp \
	src/app/LibFox/libfox.cpp \
	src/app/LibFox/photon.c

src_app_LibFox_tfox_SOURCES = \
	src/app/LibFox/app.cpp \
	src/app/LibFox/libfox.cpp

#to allow us to pass specific cflags
EXTRA_LIBRARIES += src/app/LibFox/libmcphoton.a

src_app_LibFox_libmcphoton_a_SOURCES = \
	src/app/LibFox/mcphoton.c

src_app_LibFox_libmcphoton_a_CFLAGS = -DUSE_MPI

#to allow us to pass specific cflags
EXTRA_LIBRARIES += src/app/LibFox/libtfox.a

src_app_LibFox_libtfox_a_SOURCES = \
	src/app/LibFox/tfox.c

src_app_LibFox_libtfox_a_CFLAGS = -DUSE_MPI

src_app_LibFox_mcphoton_ebbs = \
	src/ebb/EbbManager/libPrimitiveEbbManager.a

src_app_LibFox_mcphoton_LDFLAGS = \
	$(app_linkflags) \
	-Wl,-whole-archive \
	$(ebbrt_libs) \
	$(src_app_LibFox_mcphoton_ebbs) \
	-Wl,-no-whole-archive

src_app_LibFox_mcphoton_LDADD = \
	libebbrt.a \
	src/app/LibFox/libmcphoton.a

src_app_LibFox_mcphoton_DEPENDENCIES = \
	libebbrt.a \
	src/app/LibFox/libmcphoton.a \
	$(app_deps) \
	$(ebbrt_libs) \
	$(src_app_LibFox_mcphoton_ebbs)

src_app_LibFox_mcphoton_LDFLAGS += \
	-pthread

src_app_LibFox_tfox_ebbs = \
	src/ebb/EbbManager/libPrimitiveEbbManager.a

src_app_LibFox_tfox_LDFLAGS = \
	$(app_linkflags) \
	-Wl,-whole-archive \
	$(ebbrt_libs) \
	$(src_app_LibFox_tfox_ebbs) \
	-Wl,-no-whole-archive

src_app_LibFox_tfox_LDADD = \
	libebbrt.a \
	src/app/LibFox/libtfox.a

src_app_LibFox_tfox_DEPENDENCIES = \
	libebbrt.a \
	src/app/LibFox/libtfox.a \
	$(app_deps) \
	$(ebbrt_libs) \
	$(src_app_LibFox_tfox_ebbs)

src_app_LibFox_tfox_LDFLAGS += \
	-pthread
