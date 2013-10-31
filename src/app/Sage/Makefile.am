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
#to allow us to pass specific cflags
# lib_LTLIBRARIES += src/app/Sage/hello_ebb.la

# src_app_Sage_hello_ebb_la_SOURCES = \
# 	src/app/Sage/HelloEbb.cpp \
# 	src/app/Sage/hello_ebb.c

# src_app_Sage_hello_ebb_la_ebbs = \
# 	src/ebb/EbbManager/libPrimitiveEbbManager.la \
# 	src/ebb/Console/libLocalConsole.la \
# 	src/ebb/EventManager/libSimpleEventManager.la

# src_app_Sage_hello_ebb_la_LDFLAGS = \
# 	-module

# src_app_Sage_hello_ebb_la_LIBADD = \
# 	$(src_app_Sage_hello_ebb_la_ebbs) \
# 	libebbrt.la \
# 	-lpython2.7

# src_app_Sage_hello_ebb_la_DEPENDENCIES = \
# 	libebbrt.la \
# 	$(app_deps) \
# 	$(ebbrt_libs) \
# 	$(src_app_Sage_hello_ebb_la_ebbs)

# src_app_Sage_hello_ebb_la_CPPFLAGS = \
# 	$(AM_CPPFLAGS) -I /usr/include/python2.7

# lib_LTLIBRARIES += src/app/Sage/matrix_ebb_real_double_dense.la

# src_app_Sage_matrix_ebb_real_double_dense_la_SOURCES = \
# 	src/app/Sage/matrix_ebb_real_double_dense.cpp \
# 	src/app/Sage/Rectangle.cpp

# src_app_Sage_matrix_ebb_real_double_dense_la_LDFLAGS = \
# 	-module

# src_app_Sage_matrix_ebb_real_double_dense_la_LIBADD = \
# 	-lpython2.7

# SAGE_ROOT = /home/jappavoo/Work/sage/sage-5.12

CYTHON_CPPFLAGS = \
	-I $(SAGE_ROOT)/devel/sage/ \
	-I $(SAGE_ROOT)/devel/sage/sage/matrix \
	-I $(SAGE_ROOT)/devel/sage/sage/ext \
	-I $(SAGE_ROOT)/devel/sage/c_lib/include \
	-I $(SAGE_ROOT)/local/lib/python/site-packages/numpy/core/include

# src_app_Sage_matrix_ebb_real_double_dense_la_CPPFLAGS = \
# 	$(AM_CPPFLAGS) -I /usr/include/python2.7 $(CYTHON_CPPFLAGS)

# src_app_Sage_matrix_ebb_real_double_dense_la_CXXFLAGS = \
# 	$(AM_CXXFLAGS) -Wno-error=unused-function -Wno-error=cpp

%.c: %.pyx
	cython -v $(CYTHON_CPPFLAGS) $< -o $@

%.cpp: %.pyx
	cython --cplus -v $(CYTHON_CPPFLAGS) $< -o $@

if LRT_ULNX
EXTRA_LTLIBRARIES += src/app/Sage/ebb_matrix.la

src_app_Sage_ebb_matrix_la_SOURCES = \
	src/app/Sage/ebb_matrix.cpp \
	src/app/Sage/ebb_matrix_helper.cpp \
	src/app/Sage/Matrix.cpp \
	src/app/Sage/LocalMatrix.cpp

src_app_Sage_ebb_matrix_la_ebbs = \
	src/ebb/EbbManager/PrimitiveEbbManager.la \
	src/ebb/EventManager/SimpleEventManager.la \
	src/ebb/MessageManager/UDPMessageManager.la \
	src/ebb/Config/Fdt.la \
	src/ebb/NodeAllocator/Kittyhawk.la \
	src/ebb/Network/SocketNetwork.la

src_app_Sage_ebb_matrix_la_LDFLAGS = \
	-module \
	-no-undefined \
	-rpath $(abs_builddir)

src_app_Sage_ebb_matrix_la_LIBADD = \
	$(src_app_Sage_ebb_matrix_la_ebbs) \
	libebbrt.la \
	$(ebbrt_libs) \
	-lpython2.7

src_app_Sage_ebb_matrix_la_DEPENDENCIES = \
	libebbrt.la \
	$(app_deps) \
	$(ebbrt_libs) \
	src/app/Sage/SageMatrix.dtb \
	$(src_app_Sage_ebb_matrix_la_ebbs)

src_app_Sage_ebb_matrix_la_CXXFLAGS = \
	-std=c++11 -Wall
src_app_Sage_ebb_matrix_la_CPPFLAGS = \
	$(AM_CPPFLAGS) -I$(SAGE_ROOT)/devel/sage -I$(SAGE_ROOT)/devel/sage/sage/matrix -I$(SAGE_ROOT)/devel/sage/sage/ext -I$(SAGE_ROOT)/devel/sage/c_lib/include -I$(SAGE_ROOT)/local/lib/python/site-packages/numpy/core/include -I$(SAGE_ROOT)/local/include/python2.7

endif

if LRT_BARE
EXTRA_PROGRAMS += src/app/Sage/SageMatrix

src_app_Sage_SageMatrix_SOURCES = \
	src/app/Sage/ebb_matrix_helper.cpp \
	src/app/Sage/Matrix.cpp

src_app_Sage_SageMatrix_ebbs = \
	src/ebb/EbbManager/PrimitiveEbbManager.la \
	src/ebb/EventManager/SimpleEventManager.la \
	src/ebb/MessageManager/UDPMessageManager.la \
	src/ebb/MemoryAllocator/SimpleMemoryAllocator.la \
	src/ebb/Ethernet/VirtioNet.la \
	src/ebb/PCI/PCI.la \
	src/ebb/Syscall/Syscall.la \
	src/ebb/Gthread/Gthread.la \
	src/ebb/Timer/ApicTimer.la \
	src/ebb/Network/LWIPNetwork.la \
	src/ebb/Config/Fdt.la

src_app_Sage_SageMatrix_linked_ebbs = $(foreach ebb,$(src_app_Sage_SageMatrix_ebbs),$(if $(filter %.la,$(ebb)),$(dir $(ebb)).libs/$(STATIC_LIBRARY_PREFIX)$(notdir $(basename $(ebb))).a,$(ebb)))

src_app_Sage_SageMatrix_LDFLAGS = \
	$(app_linkflags) \
	-Wl,'-whole-archive,$(subst $(SPACE),$(COMMA),$(strip $(ebbrt_linked_libs) $(src_app_Sage_SageMatrix_linked_ebbs))),-no-whole-archive'

src_app_Sage_SageMatrix_LDADD = \
	libebbrt.la

src_app_Sage_SageMatrix_DEPENDENCIES = \
	libebbrt.la \
	$(app_deps) \
	$(ebbrt_libs) \
	$(src_app_Sage_SageMatrix_ebbs)
endif

# EXTRA_PROGRAMS += src/app/Sage/HelloEbbTest

# src_app_Sage_HelloEbbTest_SOURCES = \
# 	src/app/Sage/test.cpp

# src_app_Sage_HelloEbbTest_LDADD = \
# 	src/app/Sage/libHelloEbb.la
