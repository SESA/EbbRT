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

# FIXME removed since doesn't compile
EXTRA_PROGRAMS += src/app/HelloRemote/HelloRemote

src_app_HelloRemote_HelloRemote_SOURCES = \
	src/app/HelloRemote/HelloRemote.cpp

src_app_HelloRemote_HelloRemote_ebbs = \
	src/ebb/EbbManager/libPrimitiveEbbManager.a \
	src/ebb/EventManager/libSimpleEventManager.a \
	src/ebb/Console/libRemoteConsole.a

if !BG
src_app_HelloRemote_HelloRemote_ebbs += \
	src/ebb/MessageManager/libEthernetMessageManager.a
else
src_app_HelloRemote_HelloRemote_ebbs += \
	src/ebb/MessageManager/libMPIMessageManager.a
endif

if LRT_ULNX
if BG
else
src_app_HelloRemote_HelloRemote_ebbs += \
	src/ebb/Ethernet/libRawSocket.a
endif
else
if LRT_BARE
src_app_HelloRemote_HelloRemote_ebbs += \
	src/ebb/MemoryAllocator/libSimpleMemoryAllocator.a \
	src/ebb/Ethernet/libVirtioNet.a \
	src/ebb/PCI/libPCI.a \
	src/ebb/Syscall/libSyscall.a \
	src/ebb/Gthread/libGthread.a
endif
endif


src_app_HelloRemote_HelloRemote_LDFLAGS = \
	$(app_linkflags) \
	-Wl,-whole-archive \
	$(ebbrt_libs) \
	$(src_app_HelloRemote_HelloRemote_ebbs) \
	-Wl,-no-whole-archive

src_app_HelloRemote_HelloRemote_LDADD = \
	libebbrt.a

if LRT_ULNX
if !BG
src_app_HelloRemote_HelloRemote_LDADD += \
	-lpcap
endif
endif

src_app_HelloRemote_HelloRemote_DEPENDENCIES = \
	libebbrt.a \
	$(app_deps) \
	$(ebbrt_libs) \
	$(src_app_HelloRemote_HelloRemote_ebbs)
