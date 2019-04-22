#
# Top level Makefile for inception-core
#

all: inception

.PHONY: deps
deps:
	@git submodule init && git submodule update

.PHONY: inception
inception: deps
	@cd targets/inception && $(MAKE)

.PHONY: test
test: shim test_aat

.PHONY: shim
shim:
	@cd targets/libicp-shim && $(MAKE)
.PHONY: test_aat
test_aat:
	@cd tests/aat && $(MAKE)

.PHONY: clean
clean:
	@cd targets/inception && $(MAKE) clean
