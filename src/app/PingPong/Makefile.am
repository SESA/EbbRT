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

EXTRA_PROGRAMS += src/app/PingPong/PingPong

src_app_PingPong_PingPong_SOURCES =

if LRT_ULNX
src_app_PingPong_PingPong_SOURCES += \
	src/app/PingPong/PingPong_linux.cpp \
	src/app/PingPong/Echo_linux.cpp
else
if LRT_BARE
src_app_PingPong_PingPong_SOURCES += \
	src/app/PingPong/PingPong_baremetal.cpp \
	src/app/PingPong/Echo_baremetal.cpp
endif
endif

src_app_PingPong_PingPong_ebbs = \
	src/ebb/EbbManager/PrimitiveEbbManager.la \
	src/ebb/EventManager/SimpleEventManager.la \
	src/ebb/Config/Fdt.la \
	src/ebb/Console/LocalConsole.la

if UDP
src_app_PingPong_PingPong_ebbs += \
	src/ebb/MessageManager/UDPMessageManager.la
else
src_app_PingPong_PingPong_ebbs += \
	src/ebb/MessageManager/EthernetMessageManager.la
endif

if LRT_ULNX
src_app_PingPong_PingPong_ebbs += \
	src/ebb/NodeAllocator/libKittyhawk.a
if UDP
src_app_PingPong_PingPong_ebbs += \
	src/ebb/Network/SocketNetwork.la
else
src_app_PingPong_PingPong_ebbs += \
	src/ebb/Ethernet/RawSocket.la
endif
else
if LRT_BARE
src_app_PingPong_PingPong_ebbs += \
	src/ebb/MemoryAllocator/SimpleMemoryAllocator.la \
	src/ebb/Ethernet/VirtioNet.la \
	src/ebb/PCI/PCI.la \
	src/ebb/Syscall/Syscall.la \
	src/ebb/Gthread/Gthread.la

if UDP
src_app_PingPong_PingPong_ebbs += \
	src/ebb/Timer/ApicTimer.la \
	src/ebb/Network/LWIPNetwork.la
endif
endif
endif

src_app_PingPong_PingPong_linked_ebbs = $(foreach ebb,$(src_app_PingPong_PingPong_ebbs),$(if $(filter %.la,$(ebb)),$(dir $(ebb)).libs/$(STATIC_LIBRARY_PREFIX)$(notdir $(basename $(ebb))).a,$(ebb)))

src_app_PingPong_PingPong_LDFLAGS = \
	$(app_linkflags) \
	-Wl,'-whole-archive,$(subst $(SPACE),$(COMMA),$(strip $(ebbrt_linked_libs) $(src_app_PingPong_PingPong_linked_ebbs))),-no-whole-archive'

src_app_PingPong_PingPong_LDADD = \
	libebbrt.la

if LRT_ULNX
src_app_PingPong_PingPong_LDADD += \
	-lpcap
endif

src_app_PingPong_PingPong_DEPENDENCIES = \
	libebbrt.la \
	$(app_deps) \
	$(ebbrt_libs) \
	$(src_app_PingPong_PingPong_ebbs)
