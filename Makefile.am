AM_CXXFLAGS = -std=c++0x -Wall -Werror
AM_CPPFLAGS = -iquote $(top_srcdir)/src \
	--include $(top_builddir)/config.h

lib_LIBRARIES = libebbrt.a

libebbrt_a_SOURCES = \
	$(arch_sources) \
	$(app_sources) \
	$(ebb_sources) \
	$(lrt_sources) \
	$(misc_sources)
libebbrt_a_LIBADD =
libebbrt_a_DEPENDENCIES =
EXTRA_DIST =
EXTRA_LIBRARIES =
ebbrt_libs =

app_linkflags =
app_deps =

if LRT_BARE

if ARCH_X86_64
app_linkflags += -n -z max-page-size=0x1000 \
	-T $(top_srcdir)/src/lrt/bare/arch/x86_64/link.ld

%.iso: % util/grub.cfg
	strip -s $< -o $<.stripped
	grub-mkrescue -o $@ -graft-points boot/ebbrt=$<.stripped \
	boot/grub/grub.cfg=$(top_srcdir)/util/grub.cfg
endif

if ARCH_POWERPC
#powerpc build specifics
endif

#end of LRT_BARE
endif 

include src/arch/Makefile.am
include src/app/Makefile.am
include src/ebb/Makefile.am
include src/lib/Makefile.am
include src/lrt/Makefile.am
include src/misc/Makefile.am
