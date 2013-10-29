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
EXTRA_LTLIBRARIES += \
	src/ebb/MessageManager/EthernetMessageManager.la \
	src/ebb/MessageManager/MPIMessageManager.la \
	src/ebb/MessageManager/UDPMessageManager.la

src_ebb_MessageManager_EthernetMessageManager_la_SOURCES = \
	src/ebb/MessageManager/EthernetMessageManager.cpp

src_ebb_MessageManager_EthernetMessageManager_la_LDFLAGS = \
	-module

src_ebb_MessageManager_MPIMessageManager_la_SOURCES = \
	src/ebb/MessageManager/MPIMessageManager.cpp

src_ebb_MessageManager_MPIMessageManager_la_LDFLAGS = \
	-module

src_ebb_MessageManager_UDPMessageManager_la_SOURCES = \
	src/ebb/MessageManager/UDPMessageManager.cpp

src_ebb_MessageManager_UDPMessageManager_la_LDFLAGS = \
	-module
