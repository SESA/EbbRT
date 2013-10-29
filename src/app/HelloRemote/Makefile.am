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
	src/ebb/EbbManager/PrimitiveEbbManager.la \
	src/ebb/EventManager/SimpleEventManager.la \
	src/ebb/Console/RemoteConsole.la

if !BG
src_app_HelloRemote_HelloRemote_ebbs += \
	src/ebb/MessageManager/EthernetMessageManager.la
else
src_app_HelloRemote_HelloRemote_ebbs += \
	src/ebb/MessageManager/MPIMessageManager.la
endif

if LRT_ULNX
if BG
else
src_app_HelloRemote_HelloRemote_ebbs += \
	src/ebb/Ethernet/RawSocket.la
endif
else
if LRT_BARE
src_app_HelloRemote_HelloRemote_ebbs += \
	src/ebb/MemoryAllocator/SimpleMemoryAllocator.la \
	src/ebb/Ethernet/VirtioNet.la \
	src/ebb/PCI/PCI.la \
	src/ebb/Syscall/Syscall.la \
	src/ebb/Gthread/Gthread.la
endif
endif

src_app_HelloRemote_HelloRemote_linked_ebbs = $(foreach ebb,$(src_app_HelloRemote_HelloRemote_ebbs),$(if $(filter %.la,$(ebb)),$(dir $(ebb)).libs/$(STATIC_LIBRARY_PREFIX)$(notdir $(basename $(ebb))).a,$(ebb)))

src_app_HelloRemote_HelloRemote_LDFLAGS = \
	$(app_linkflags) \
	-Wl,'-whole-archive,$(subst $(SPACE),$(COMMA),$(strip $(ebbrt_linked_libs) $(src_app_HelloRemote_HelloRemote_linked_ebbs))),-no-whole-archive'

src_app_HelloRemote_HelloRemote_LDADD = \
	libebbrt.la

if LRT_ULNX
if !BG
src_app_HelloRemote_HelloRemote_LDADD += \
	-lpcap
endif
endif

src_app_HelloRemote_HelloRemote_DEPENDENCIES = \
	libebbrt.la \
	$(app_deps) \
	$(ebbrt_libs) \
	$(src_app_HelloRemote_HelloRemote_ebbs)
