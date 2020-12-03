# Top-level packaging Makefile

.PHONY: \
	all \
	package \
	package-deb \
	clean \
	clean-deb

all: package

package: package-deb

package-deb:
	cd deb && $(MAKE) all

clean-deb:
	cd deb && $(MAKE) clean

clean: clean-deb
