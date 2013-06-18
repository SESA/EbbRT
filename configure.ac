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
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror -Wno-portability])
#some random file which we assert exists in source tree
AC_CONFIG_SRCDIR([src/ebbrt.hpp])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])
AM_PATH_PYTHON([2.6],, [:])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != ":"])

m4_include([m4/doxy.m4])
DX_HTML_FEATURE(ON)
DX_INIT_DOXYGEN($PACKAGE_NAME, docs/Doxyfile)

AC_CANONICAL_HOST
AS_CASE([$host_cpu], [x86_64],
                     [AC_DEFINE([ARCH_X86_64], [1], [x86_64 architecture])],
                     [powerpc],
                     [AC_DEFINE([ARCH_POWERPC], [1], [powerpc32 architecture])],
                     [AC_MSG_ERROR([not on a supported architecture])])
AM_CONDITIONAL([ARCH_X86_64], [test x$host_cpu == xx86_64])
AM_CONDITIONAL([ARCH_POWERPC], [test x$host_cpu == xpowerpc])

# Baremetal or Linux build
AC_ARG_ENABLE([baremetal],
  AS_HELP_STRING([--enable-baremetal], [Enable baremetal build]),
  [lrt=bare],
  [lrt=ulnx])

AS_CASE([$lrt],
        [bare], [AC_DEFINE([LRT_BARE], [1], [Baremetal LRT])],
                    [ulnx], [AC_DEFINE([LRT_ULNX], [1], [Baremetal ULNX])])

AM_CONDITIONAL(LRT_BARE, test x$lrt = xbare)
AM_CONDITIONAL(LRT_ULNX, test x$lrt = xulnx)

# Checks for programs.
AM_PROG_AS
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

AC_OUTPUT
