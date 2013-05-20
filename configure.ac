AC_PREREQ(2.61)
AC_INIT([EbbRT],[0.3],[sesa-developers@googlegroups.com])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror -Wno-portability])
#some random file which we assert exists in source tree
AC_CONFIG_SRCDIR([src/lrt/bare/boot.cpp])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])

AC_CANONICAL_HOST
AS_CASE([$host_cpu], [x86_64],
[AC_DEFINE([ARCH_X86_64], [1], [x86_64 architecture])],
[powerpc],
[AC_DEFINE([ARCH_PPC], [1], [powerpc architecture])],
[AC_MSG_ERROR([not on a supported architecture])])
AM_CONDITIONAL([ARCH_X86_64], [test x$host_cpu == xx86_64])
AM_CONDITIONAL([ARCH_POWERPC], [test x$host_cpu == xpowerpc])

# Baremetal or Linux 
AC_ARG_ENABLE([baremetal],
  AS_HELP_STRING([--enable-baremetal], [Enable baremetal build]),
  [lrt=bare],
  [lrt=ulnx])

AS_CASE([$lrt], [bare], [AC_DEFINE([LRT_BARE], [1], [Baremetal LRT])],
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

# Checks for library functions.

AC_OUTPUT
