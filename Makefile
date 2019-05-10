#
# Top level Makefile for inception-core
#

all: inception libicp-shim

.PHONY: deps
deps:
	@git submodule init && git submodule update

.PHONY: inception
inception: deps
	@cd targets/inception && $(MAKE)

.PHONY: libicp-shim
libicp-shim: deps
	@cd targets/libicp-shim && $(MAKE)

.PHONY: test
test: test_unit test_aat

.PHONY: test_aat
test_aat: inception
	@cd tests/aat && $(MAKE)

.PHONY: test_unit
test_unit:
	@cd tests/unit && $(MAKE)

.PHONY: clean
clean:
	@cd targets/inception && $(MAKE) clean
	@cd targets/libicp-shim && $(MAKE) clean
	@cd tests/aat && $(MAKE) clean
	@cd tests/unit && $(MAKE) clean
