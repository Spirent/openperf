# Top-level packaging Makefile

# Make variables available to sub-make files
export PKG_VERSION ?= 1
export PKG_BASE_NAME := openperf

.PHONY: \
	all \
	package \
	package-debian \
	package-rpm \
	clean \
	clean-debian \
	clean-rpm

all: package

package: package-debian package-rpm

package-debian:
	cd debian && $(MAKE) all

package-rpm:
	cd rpm && $(MAKE) all

clean-debian:
	cd debian && $(MAKE) clean

clean-rpm:
	cd rpm && $(MAKE) clean

clean: clean-debian clean-rpm
