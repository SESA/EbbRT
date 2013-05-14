AM_CXXFLAGS = -std=c++0x -Wall -Werror
AM_CPPFLAGS = -iquote $(top_srcdir)/src \
	--include $(top_builddir)/config.h

bin_PROGRAMS = ebbrt
ebbrt_SOURCES = \
	$(arch_sources) \
	$(app_sources) \
	$(ebb_sources) \
	$(lrt_sources)
ebbrt_LDFLAGS = -u _start
ebbrt_DEPENDENCIES =
EXTRA_DIST =
noinst_LIBRARIES =

if ARCH_X86_64
ebbrt_LDFLAGS += -static -n -z max-page-size=0x1000 \
	-T $(top_srcdir)/src/lrt/arch/x86_64/link.ld

# ebbrt.iso: ebbrt util/grub.cfg
#	strip -d $< -o $<.stripped
#	grub-mkrescue -o $@ -graft-points boot/ebbrt=$<.stripped \
#	boot/grub/grub.cfg=$(top_srcdir)/util/grub.cfg

ebbrt.iso: ebbrt config util/grub.cfg
	ebbrt-grub-mkrescue -o $@ -graft-points boot/ebbrt=ebbrt \
	boot/config=config boot/grub/grub.cfg=$(top_srcdir)/util/grub.cfg
endif

include src/arch/Makefile.am
include src/app/Makefile.am
include src/ebb/Makefile.am
include src/lib/Makefile.am
include src/lrt/Makefile.am
