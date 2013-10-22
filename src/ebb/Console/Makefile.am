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
	src/ebb/Console/libRemoteConsole.la \
	src/ebb/Console/libLocalConsole.la

src_ebb_Console_libRemoteConsole_la_SOURCES = \
	src/ebb/Console/Console.hpp \
	src/ebb/Console/RemoteConsole.cpp \
	src/ebb/Console/RemoteConsole.hpp

src_ebb_Console_libRemoteConsole_la_LDFLAGS = -rpath /nowhere

src_ebb_Console_libLocalConsole_la_SOURCES = \
	src/ebb/Console/Console.hpp \
	src/ebb/Console/LocalConsole.cpp \
	src/ebb/Console/LocalConsole.hpp

src_ebb_Console_libLocalConsole_la_LDFLAGS = -rpath /nowhere
