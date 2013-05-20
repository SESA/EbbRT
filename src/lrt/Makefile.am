include src/lrt/bare/Makefile.am
include src/lrt/ulnx/Makefile.am

lrt_sources = \
	src/lrt/boot.hpp \
	src/lrt/console.hpp \
	src/lrt/event.hpp \
	src/lrt/mem.hpp \
	src/lrt/trans.hpp 

if LRT_ULNX
lrt_sources += $(lrt_ulnx_sources)
else
if LRT_BARE
lrt_sources += $(lrt_bare_sources)
endif
endif
