MYDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

-include config.mk

TARGET ?= x86_64-pc-ebbrt

AUTOCONF_VERSION ?= 2.64
AUTOMAKE_VERSION ?= 1.11.1
BINUTILS_VERSION ?= 2.26
CAPNPDEP_VERSION ?= 0.4.0
GCC_VERSION ?= 5.3.0
NEWLIB_VERSION ?= 2.0.0

AUTOCONF ?= autoconf
AUTOMAKE ?= automake
AUTORECONF ?= autoreconf
CD ?= cd
CMAKE ?= cmake
CP ?= cp
MAKE ?= make
MKDIR ?= mkdir
TAR ?= tar
TOUCH ?= touch
WGET ?= wget
PATCH ?= patch

SYSROOT ?= $(abspath $(CURDIR)/sysroot)
PREFIX ?= $(abspath $(SYSROOT)/usr)
EBBRT_SRCDIR ?= $(abspath $(MYDIR)/../)
SRCDIR ?= $(abspath $(CURDIR)/src)
BUILDDIR ?= $(abspath $(CURDIR)/build)
PATCHDIR ?= $(abspath $(MYDIR)/patches)
DEPS_PREFIX ?= $(abspath $(CURDIR)/deps)

AUTOCONF_FULL ?= autoconf-$(AUTOCONF_VERSION)
AUTOCONF_TAR ?= $(AUTOCONF_FULL).tar.xz
AUTOCONF_TAR_PATH ?= $(SRCDIR)/$(AUTOCONF_TAR)
AUTOCONF_SRC_PATH ?= $(SRCDIR)/$(AUTOCONF_FULL)
AUTOCONF_BUILD_PATH ?= $(BUILDDIR)/autoconf-build
AUTOCONF_URL ?= https://ftp.gnu.org/gnu/autoconf/$(AUTOCONF_TAR)
AUTOCONF_CONFIGURE_FLAG ?= $(AUTOCONF_BUILD_PATH)/Makefile

AUTOMAKE_FULL ?= automake-$(AUTOMAKE_VERSION)
AUTOMAKE_TAR ?= $(AUTOMAKE_FULL).tar.bz2
AUTOMAKE_TAR_PATH ?= $(SRCDIR)/$(AUTOMAKE_TAR)
AUTOMAKE_SRC_PATH ?= $(SRCDIR)/$(AUTOMAKE_FULL)
AUTOMAKE_BUILD_PATH ?= $(BUILDDIR)/automake-build
AUTOMAKE_URL ?= https://ftp.gnu.org/gnu/automake/$(AUTOMAKE_TAR)
AUTOMAKE_CONFIGURE_FLAG ?= $(AUTOMAKE_BUILD_PATH)/Makefile

CAPNPDEP ?= capnproto-c++-$(CAPNPDEP_VERSION)
CAPNPDEP_TAR ?= $(CAPNPDEP).tar.gz
CAPNPDEP_PATCH ?= $(CAPNPDEP).patch
CAPNPDEP_TAR_PATH ?= $(SRCDIR)/$(CAPNPDEP_TAR)
CAPNPDEP_SRC_PATH ?= $(SRCDIR)/$(CAPNPDEP)
CAPNPDEP_BUILD_PATH ?= $(BUILDDIR)/capnpdep-build
CAPNPDEP_PATCH_PATH ?= $(PATCHDIR)/$(CAPNPDEP_PATCH)
CAPNPDEP_URL ?= https://capnproto.org/$(CAPNPDEP_TAR)
CAPNPDEP_CONFIGURE_FLAG ?=  $(CAPNPDEP_BUILD_PATH)/Makefile

BINUTILS ?= binutils-$(BINUTILS_VERSION)
BINUTILS_TAR ?= $(BINUTILS).tar.bz2
BINUTILS_PATCH ?= $(BINUTILS).patch
BINUTILS_TAR_PATH ?= $(SRCDIR)/$(BINUTILS_TAR)
BINUTILS_SRC_PATH ?= $(SRCDIR)/$(BINUTILS)
BINUTILS_BUILD_PATH ?= $(BUILDDIR)/binutils-build
BINUTILS_PATCH_PATH ?= $(PATCHDIR)/$(BINUTILS_PATCH)
BINUTILS_URL ?= https://ftp.gnu.org/gnu/binutils/$(BINUTILS_TAR)
BINUTILS_CONFIGURE_FLAG ?= $(BINUTILS_BUILD_PATH)/Makefile

GCC ?= gcc-$(GCC_VERSION)
GCC_TAR ?= $(GCC).tar.bz2
GCC_PATCH ?= $(GCC).patch
GCC_TAR_PATH ?= $(SRCDIR)/$(GCC_TAR)
GCC_SRC_PATH ?= $(SRCDIR)/$(GCC)
GCC_BUILD_PATH ?= $(BUILDDIR)/gcc-build
GCC_PATCH_PATH ?= $(PATCHDIR)/$(GCC_PATCH)
GCC_URL ?= https://ftp.gnu.org/gnu/gcc/$(GCC)/$(GCC_TAR)
GCC_CONFIGURE_FLAG ?= $(GCC_BUILD_PATH)/Makefile

NEWLIB ?= newlib-$(NEWLIB_VERSION)
NEWLIB_TAR ?= $(NEWLIB).tar.gz
NEWLIB_PATCH ?= $(NEWLIB).patch
NEWLIB_TAR_PATH ?= $(SRCDIR)/$(NEWLIB_TAR)
NEWLIB_SRC_PATH ?= $(SRCDIR)/$(NEWLIB)
NEWLIB_BUILD_PATH ?= $(BUILDDIR)/newlib-build
NEWLIB_PATCH_PATH ?= $(PATCHDIR)/$(NEWLIB_PATCH)
NEWLIB_URL ?= ftp://sourceware.org/pub/newlib/$(NEWLIB_TAR)
NEWLIB_CONFIGURE_FLAG ?= $(NEWLIB_BUILD_PATH)/Makefile
ifdef DEBUG
NEWLIB_CFLAGS ?= "-O0 -g3"
else
NEWLIB_CFLAGS ?= "-O4" # FIXME: Link Time Optimization fails
endif


EBBRT_BM_PATH ?= $(EBBRT_SRCDIR)/src/native
EBBRT_BUILD_PATH ?= $(BUILDDIR)/ebbrt-build
EBBRT_CONFIGURE_FLAG ?= $(EBBRT_BUILD_PATH)/Makefile

CMAKE_TOOLCHAIN_FILE ?= $(EBBRT_SRCDIR)/src/cmake/ebbrt.cmake
ifdef DEBUG
CMAKE_OPT ?= -DCMAKE_BUILD_TYPE=Debug
else
CMAKE_OPT ?= -DCMAKE_BUILD_TYPE=Release
endif
CMAKE_DEFS ?= -DCMAKE_INSTALL_PREFIX:PATH=$(PREFIX) \
	-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE) $(CMAKE_OPT)
EBBRT_BUILD_DEFS ?= -DCMAKE_C_COMPILER_FORCED=1 \
										-DCMAKE_CXX_COMPILER_FORCED=1

ACPICA_SRC_PATH ?= $(abspath $(MYDIR)/ext/acpica)
ACPICA_BUILD_PATH ?= $(BUILDDIR)/acpica-build
ACPICA_CONFIGURE_FLAG ?= $(ACPICA_BUILD_PATH)/Makefile

BOOST_SRC_PATH ?= $(abspath $(MYDIR)/ext/boost)
BOOST_BUILD_PATH ?= $(BUILDDIR)/boost-build
BOOST_CONFIGURE_FLAG ?= $(BOOST_BUILD_PATH)/Makefile

CAPNP_SRC_PATH ?= $(abspath $(MYDIR)/ext/capnp)
CAPNP_BUILD_PATH ?= $(BUILDDIR)/capnp-build
CAPNP_CONFIGURE_FLAG ?= $(CAPNP_BUILD_PATH)/Makefile

TBB_SRC_PATH ?= $(abspath $(MYDIR)/ext/tbb)
TBB_BUILD_PATH ?= $(BUILDDIR)/tbb-build
TBB_CONFIGURE_FLAG ?= $(TBB_BUILD_PATH)/Makefile

export PATH := $(DEPS_PREFIX)/bin:$(PREFIX)/bin:$(PATH)
export EBBRT_SYSROOT := $(SYSROOT)

all: ebbrt-install

print-%  : ; @echo $* = $($*)

clean:
	-$(RM) -r $(SRCDIR) $(BUILDDIR) $(SYSROOT) $(DEPS_PREFIX)

$(SRCDIR):
	$(MKDIR) $@

$(BUILDDIR):
	$(MKDIR) $@

$(SYSROOT):
	$(MKDIR) $@

$(PREFIX): | $(SYSROOT)
	$(MKDIR) $@

### Autoconf ###

$(AUTOCONF_SRC_PATH): | $(AUTOCONF_TAR_PATH) $(SRCDIR)
	$(TAR) -xf $(AUTOCONF_TAR_PATH) -C $(SRCDIR)

$(AUTOCONF_TAR_PATH): | $(SRCDIR)
	$(WGET) $(AUTOCONF_URL) -O $@

$(AUTOCONF_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(AUTOCONF_CONFIGURE_FLAG): | $(AUTOCONF_SRC_PATH) $(AUTOCONF_BUILD_PATH)
	$(CD) $(AUTOCONF_BUILD_PATH) && $(AUTOCONF_SRC_PATH)/configure \
		--prefix=$(DEPS_PREFIX)

autoconf-build: $(AUTOCONF_CONFIGURE_FLAG)
	$(MAKE) -C $(AUTOCONF_BUILD_PATH)

autoconf-install: autoconf-build
	$(MAKE) install -C $(AUTOCONF_BUILD_PATH)

autoconf: autoconf-install

### Automake ###

$(AUTOMAKE_SRC_PATH): | $(AUTOMAKE_TAR_PATH) $(SRCDIR)
	$(TAR) -xf $(AUTOMAKE_TAR_PATH) -C $(SRCDIR)

$(AUTOMAKE_TAR_PATH): | $(SRCDIR)
	$(WGET) $(AUTOMAKE_URL) -O $@

$(AUTOMAKE_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(AUTOMAKE_CONFIGURE_FLAG): | $(AUTOMAKE_SRC_PATH) $(AUTOMAKE_BUILD_PATH)
	$(CD) $(AUTOMAKE_BUILD_PATH) && $(AUTOMAKE_SRC_PATH)/configure \
		--prefix=$(DEPS_PREFIX)

automake-build: $(AUTOMAKE_CONFIGURE_FLAG)
	$(MAKE) -C $(AUTOMAKE_BUILD_PATH)

automake-install: automake-build
	$(MAKE) install -C $(AUTOMAKE_BUILD_PATH)

automake: automake-install

### Capnp ###

$(CAPNPDEP_SRC_PATH): $(CAPNPDEP_PATCH_PATH) | $(CAPNPDEP_TAR_PATH) $(SRCDIR)
	-$(RM) -r $(CAPNPDEP_SRC_PATH)
	$(TAR) -xf $(CAPNPDEP_TAR_PATH) -C $(SRCDIR)
	$(PATCH) -p1 -l -s -d $@ < $(CAPNPDEP_PATCH_PATH)

$(CAPNPDEP_TAR_PATH): | $(SRCDIR)
	$(WGET) $(CAPNPDEP_URL) -O $@

$(CAPNPDEP_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(CAPNPDEP_CONFIGURE_FLAG): | $(CAPNPDEP_SRC_PATH) $(CAPNPDEP_BUILD_PATH)
	$(CD) $(CAPNPDEP_BUILD_PATH) && $(CAPNPDEP_SRC_PATH)/configure \
		--prefix=$(DEPS_PREFIX)

capnpdep-build: $(CAPNPDEP_CONFIGURE_FLAG)
	$(MAKE) -C $(CAPNPDEP_BUILD_PATH)

capnpdep-install: capnpdep-build
	$(MAKE) install -C $(CAPNPDEP_BUILD_PATH)

capnpdep: capnpdep-install

### deps ###

deps: autoconf automake capnpdep

### Binutils ###

$(BINUTILS_SRC_PATH): $(BINUTILS_PATCH_PATH) | deps $(BINUTILS_TAR_PATH) $(SRCDIR)
	-$(RM) -r $(BINUTILS_SRC_PATH)
	$(TAR) -xf $(BINUTILS_TAR_PATH) -C $(SRCDIR)
	$(PATCH) -p1 -l -s -d $@ < $(BINUTILS_PATCH_PATH)
	cd $@/ld && $(AUTOMAKE)

$(BINUTILS_TAR_PATH): | $(SRCDIR)
	$(WGET) $(BINUTILS_URL) -O $@

$(BINUTILS_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(BINUTILS_CONFIGURE_FLAG): | $(BINUTILS_SRC_PATH) $(BINUTILS_BUILD_PATH)
	$(CD) $(BINUTILS_BUILD_PATH) && $(BINUTILS_SRC_PATH)/configure \
		--target=$(TARGET) --prefix=$(PREFIX) \
		--with-sysroot=$(SYSROOT) --disable-werror --disable-nls \
		--enable-plugins

binutils-build: $(BINUTILS_CONFIGURE_FLAG)
	$(MAKE) -C $(BINUTILS_BUILD_PATH)

binutils-install: binutils-build
	$(MAKE) install -C $(BINUTILS_BUILD_PATH)

binutils: binutils-install

### GCC ###

$(GCC_SRC_PATH): $(GCC_PATCH_PATH) | deps $(GCC_TAR_PATH) $(SRCDIR)
	-$(RM) -r $(GCC_SRC_PATH)
	$(TAR) -xf $(GCC_TAR_PATH) -C $(SRCDIR)
	$(CD) $@ && ./contrib/download_prerequisites
	$(PATCH) -p1 -l -s -d $@ < $(GCC_PATCH_PATH) && \
	$(CD) $@/libstdc++-v3 && $(AUTOCONF) && \
	$(CD) $@/libgcc && $(AUTOCONF) && \
	$(CD) $@/gcc && $(AUTOCONF)

$(GCC_TAR_PATH): | $(SRCDIR)
	$(WGET) $(GCC_URL) -O $@

$(GCC_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(GCC_CONFIGURE_FLAG): | binutils $(GCC_SRC_PATH) $(GCC_BUILD_PATH)
	$(CD) $(GCC_BUILD_PATH) && \
	$(GCC_SRC_PATH)/configure --target=$(TARGET) --prefix=$(PREFIX) \
		--with-sysroot=$(SYSROOT) --enable-languages=c,c++ \
		--disable-libssp --without-headers --disable-nls --with-newlib \
		--enable-threads=ebbrt

gcc-build: binutils ebbrt-headers $(GCC_CONFIGURE_FLAG)
	$(MAKE) all-gcc  -C $(GCC_BUILD_PATH)

gcc-install: gcc-build
	$(MAKE) install-gcc -C $(GCC_BUILD_PATH)

gcc: gcc-install

gcc-all-build: binutils gcc-install newlib-install
	$(MAKE) -C $(GCC_BUILD_PATH) all

gcc-all-install: gcc-all-build
	$(MAKE) install -C $(GCC_BUILD_PATH)

gcc-all: gcc-all-install

### Newlib ###

$(NEWLIB_SRC_PATH): $(NEWLIB_PATCH_PATH) | deps $(NEWLIB_TAR_PATH) $(SRCDIR)
	-$(RM) -r $(NEWLIB_SRC_PATH)
	$(TAR) -xf $(NEWLIB_TAR_PATH) -C $(SRCDIR)
	$(PATCH) -p1 -l -s -d $@ < $(NEWLIB_PATCH_PATH) && \
	$(CD) $@/newlib/libc/sys && $(AUTOCONF) && \
	$(CD) $@/newlib/libc/sys/ebbrt && $(AUTORECONF)

$(NEWLIB_TAR_PATH): | $(SRCDIR)
	$(WGET) $(NEWLIB_URL) -O $@

$(NEWLIB_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(NEWLIB_CONFIGURE_FLAG): | binutils gcc $(NEWLIB_SRC_PATH) $(NEWLIB_BUILD_PATH)
	$(CD) $(NEWLIB_BUILD_PATH) && \
	CFLAGS=$(NEWLIB_CFLAGS) AR_FOR_TARGET=x86_64-pc-ebbrt-gcc-ar \
		RANLIB_FOR_TARGET=x86_64-pc-ebbrt-gcc-ranlib \
		$(NEWLIB_SRC_PATH)/configure --target=$(TARGET) \
		--prefix=$(PREFIX) --enable-newlib-hw-fp

newlib-build: binutils gcc $(NEWLIB_CONFIGURE_FLAG)
	AR_FOR_TARGET=x86_64-pc-ebbrt-gcc-ar \
		RANLIB_FOR_TARGET=x86_64-pc-ebbrt-gcc-ranlib \
		$(MAKE) -C $(NEWLIB_BUILD_PATH)

newlib-install: newlib-build
	AR_FOR_TARGET=x86_64-pc-ebbrt-gcc-ar \
		RANLIB_FOR_TARGET=x86_64-pc-ebbrt-gcc-ranlib \
		$(MAKE) install -C $(NEWLIB_BUILD_PATH)


newlib: newlib-install

### ACPICA ###

$(ACPICA_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(ACPICA_CONFIGURE_FLAG): | gcc-all-install $(ACPICA_BUILD_PATH)
	$(CD) $(ACPICA_BUILD_PATH) && \
	$(CMAKE) $(CMAKE_DEFS) $(ACPICA_SRC_PATH)

acpica-build: gcc-all $(ACPICA_CONFIGURE_FLAG)
	$(MAKE) -C $(ACPICA_BUILD_PATH)

acpica-install: acpica-build
	$(MAKE) -C $(ACPICA_BUILD_PATH) install

### BOOST ###

$(BOOST_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(BOOST_CONFIGURE_FLAG): | gcc-all $(BOOST_BUILD_PATH)
	$(CD) $(BOOST_BUILD_PATH) && \
	$(CMAKE) $(CMAKE_DEFS) $(BOOST_SRC_PATH)

boost-build: gcc-all $(BOOST_CONFIGURE_FLAG)
	$(MAKE) -C $(BOOST_BUILD_PATH)

boost-install: boost-build
	$(MAKE) -C $(BOOST_BUILD_PATH) install

### CAPNP ###

$(CAPNP_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(CAPNP_CONFIGURE_FLAG): | gcc-all $(CAPNP_BUILD_PATH)
	$(CD) $(CAPNP_BUILD_PATH) && \
	$(CMAKE) $(CMAKE_DEFS) $(CAPNP_SRC_PATH)

capnp-build: gcc-all $(CAPNP_CONFIGURE_FLAG)
	$(MAKE) -C $(CAPNP_BUILD_PATH)

capnp-install: capnp-build
	$(MAKE) -C $(CAPNP_BUILD_PATH) install

### TBB ###

$(TBB_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(TBB_CONFIGURE_FLAG): | gcc-all $(TBB_BUILD_PATH)
	$(CD) $(TBB_BUILD_PATH) && \
	$(CMAKE) $(CMAKE_DEFS) $(TBB_SRC_PATH)

tbb-build: gcc-all $(TBB_CONFIGURE_FLAG)
	$(MAKE) -C $(TBB_BUILD_PATH)

tbb-install: tbb-build
	$(MAKE) -C $(TBB_BUILD_PATH) install

### EbbRT ###

ebbrt-deps: acpica-install boost-install capnp-install tbb-install

$(EBBRT_BUILD_PATH): | $(BUILDDIR)
	$(MKDIR) $@

$(EBBRT_CONFIGURE_FLAG): | ebbrt-deps $(EBBRT_BM_PATH) $(EBBRT_BUILD_PATH)
	$(CD) $(EBBRT_BUILD_PATH) && \
	$(CMAKE) $(CMAKE_DEFS) $(EBBRT_BUILD_DEFS) $(EBBRT_SRCDIR)/src

ebbrt-build: ebbrt-deps $(EBBRT_CONFIGURE_FLAG)
	$(MAKE) -C $(EBBRT_BUILD_PATH)

ebbrt-only:
	$(MAKE) -C $(EBBRT_BUILD_PATH) install

ebbrt-install: ebbrt-build ebbrt-cmake
	$(MAKE) -C $(EBBRT_BUILD_PATH) install

$(PREFIX)/include: | $(PREFIX)
	$(MKDIR) $@

$(PREFIX)/include/ebbrt: | $(PREFIX)/include
	$(MKDIR) $@

$(PREFIX)/include/ebbrt/native: | $(PREFIX)/include/ebbrt
	$(MKDIR) $@

$(PREFIX)/misc: | $(PREFIX)
	$(MKDIR) $@

ebbrt-headers: | $(PREFIX)/include/ebbrt/native
	$(CP) -rv $(EBBRT_SRCDIR)/src/*.h $(PREFIX)/include/ebbrt
	$(CP) -rv $(EBBRT_SRCDIR)/src/native/*.h $(PREFIX)/include/ebbrt/native

ebbrt-cmake: | $(PREFIX)/misc
	$(CP) -v $(CMAKE_TOOLCHAIN_FILE) $(PREFIX)/misc

###

.PHONY: all autoconf autoconf-build autoconf-install automake automake-build \
	automake-install binutils binutils-build binutils-install clean deps \
	gcc gcc-build gcc-install gcc-all gcc-all-build gcc-all-install newlib \
	newlib-build newlib-install acpica-build acpica-install boost-build \
	boost-install capnp-build capnp-install tbb-build tbb-install \
	ebbrt-deps ebbrt-build ebbrt-install ebbrt-cmake
