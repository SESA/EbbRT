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
AC_PREREQ(2.61)
AC_INIT([EbbRT],[0.3],[sesa-developers@googlegroups.com])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror -Wno-portability])
#some random file which we assert exists in source tree
AC_CONFIG_SRCDIR([src/ebbrt.hpp])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])
AM_PATH_PYTHON([2.6],, [:])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != ":"])

DX_HTML_FEATURE(ON)
DX_INIT_DOXYGEN($PACKAGE_NAME, docs/Doxyfile)

# Confirm host architecture and platform
AC_CANONICAL_HOST

AS_CASE([$host_cpu], [x86_64],
                     [AC_DEFINE([ARCH_X86_64], [1], [x86_64 architecture])],
                     [powerpc64],
                     [AC_DEFINE([ARCH_POWERPC64], [1], [powerpc64 architecture])],
                     [AC_MSG_ERROR([not on a supported architecture: $host_cpu])])

AM_CONDITIONAL([ARCH_X86_64], [test x$host_cpu == xx86_64])
AM_CONDITIONAL([ARCH_POWERPC64], [test x$host_cpu == xpowerpc64])

AS_CASE([$host_os], [linux-gnu],
                    [AC_DEFINE([LRT_ULNX], [1], [Linux Build])],
                    [ebbrt],
                    [AC_DEFINE([LRT_BARE], [1], [Baremetal Build])],
                    [AC_MSG_ERROR([not on a supported platform: $host_os])])

AM_CONDITIONAL([LRT_BARE], [test x$host_os == xebbrt])
AM_CONDITIONAL([LRT_ULNX], [test x$host_os == xlinux-gnu])

AC_EGREP_CPP(yes,
[#ifdef __bg__
  yes
#endif
], is_bg=yes, is_bg=no)

AM_CONDITIONAL([BG], [test x$is_bg == xyes])

AX_HAVE_EPOLL(
        [AX_CONFIG_FEATURE_ENABLE(epoll)],
        [AX_CONFIG_FEATURE_DISABLE(epoll)])

AX_CONFIG_FEATURE(
        [epoll], [This platform supports epoll(7)],
        [HAVE_EPOLL], [This platform supports epoll(7).])

# Checks for programs.
AM_PROG_AS
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

AC_OUTPUT
