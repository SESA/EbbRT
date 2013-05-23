include src/app/bare/Makefile.am
include src/app/ulnx/Makefile.am

app_sources = \
	src/app/app.hpp \
	src/app/AppEbb.hpp

EXTRA_PROGRAMS =

if LRT_ULNX
app_sources += $(app_ulnx_sources)
include src/app/FrontEnd/Makefile.am
else
if LRT_BARE
app_sources += $(app_bare_sources)
include src/app/HelloWorld/Makefile.am
include src/app/HelloRemote/Makefile.am
endif
endif
