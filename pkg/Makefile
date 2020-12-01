# Top-level packaging Makefile

.PHONY: \
	all \
	package \
	package-deb \
	package-rpm \
	clean \
	clean-deb \
	clean-rpm

all: package

package: package-deb package-rpm

package-deb:
	cd deb && $(MAKE) all

package-rpm:
	cd rpm && $(MAKE) all

clean-deb:
	cd deb && $(MAKE) clean

clean-rpm:
	cd rpm && $(MAKE) clean

clean: clean-deb clean-rpm
