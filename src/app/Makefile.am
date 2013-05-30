app_sources = \
	src/app/app.cpp \
	src/app/app.hpp \
	src/app/AppEbb.hpp

EXTRA_PROGRAMS =

include src/app/HelloRemote/Makefile.am
if LRT_ULNX
app_sources += $(app_ulnx_sources)
include src/app/FrontEnd/Makefile.am
else
if LRT_BARE
app_sources += $(app_bare_sources)
include src/app/HelloWorld/Makefile.am
endif
endif
