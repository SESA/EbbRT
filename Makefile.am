AM_CXXFLAGS = -std=c++0x -Wall -Werror
AM_CPPFLAGS = -iquote $(top_srcdir)/src \
	--include $(top_builddir)/config.h

bin_PROGRAMS = ebbrt
ebbrt_SOURCES = \
	$(lrt_sources)
ebbrt_LDFLAGS = -u _start

if ARCH_X86
ebbrt_LDFLAGS += -static -n -z max-page-size=0x1000 \
	-T $(top_srcdir)/src/lrt/arch/x86_64/link.ld

ebbrt.iso: ebbrt util/grub.cfg
	strip -d $< -o $<.stripped
	grub-mkrescue -o $@ -graft-points boot/ebbrt=$<.stripped \
	boot/grub/grub.cfg=$(top_srcdir)/util/grub.cfg
endif

include src/arch/Makefile.am
include src/lrt/Makefile.am
