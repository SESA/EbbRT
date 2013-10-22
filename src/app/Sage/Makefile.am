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
EXTRA_LTLIBRARIES += src/app/Sage/libHelloEbb.la

src_app_Sage_libHelloEbb_la_SOURCES = \
	src/app/Sage/HelloEbb.cpp

src_app_Sage_libHelloEbb_la_ebbs = \
	src/ebb/EbbManager/libPrimitiveEbbManager.la \
	src/ebb/Console/libLocalConsole.la \
	src/ebb/EventManager/libSimpleEventManager.la

src_app_Sage_libHelloEbb_la_LIBADD = \
	libebbrt.la \
	$(src_app_Sage_libHelloEbb_la_ebbs)

src_app_Sage_libHelloEbb_la_DEPENDENCIES = \
	libebbrt.la \
	$(app_deps) \
	$(ebbrt_libs) \
	$(src_app_Sage_libHelloEbb_la_ebbs)

src_app_Sage_libHelloEbb_la_LDFLAGS = -rpath /nowhere
