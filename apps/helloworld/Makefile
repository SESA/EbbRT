%::
	$(MAKE) -C hosted/build  $@
	$(MAKE) -C baremetal/build $@

.PHONY: all Release Debug

all: Release

Release:
	$(MAKE) -C hosted/build/Release
Debug:
	$(MAKE) -C hosted/build/Debug
