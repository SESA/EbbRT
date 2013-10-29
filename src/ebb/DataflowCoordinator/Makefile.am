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
	src/ebb/DataflowCoordinator/SingleDataflowCoordinator.la

src_ebb_DataflowCoordinator_SingleDataflowCoordinator_la_SOURCES = \
	src/ebb/DataflowCoordinator/SingleDataflowCoordinator.cpp

src_ebb_DataflowCoordinator_SingleDataflowCoordinator_la_LDFLAGS = \
	-module

src/ebb/DataflowCoordinator/SingleDataflowCoordinator.$(OBJEXT): \
	src/ebb/DataflowCoordinator/SingleDataflowCoordinator.pb.h

nodist_src_ebb_DataflowCoordinator_SingleDataflowCoordinator_la_SOURCES = \
	src/ebb/DataflowCoordinator/SingleDataflowCoordinator.pb.cc \
	src/ebb/DataflowCoordinator/SingleDataflowCoordinator.pb.h

CLEANFILES += \
	src/ebb/DataflowCoordinator/SingleDataflowCoordinator.pb.cc \
	src/ebb/DataflowCoordinator/SingleDataflowCoordinator.pb.h
