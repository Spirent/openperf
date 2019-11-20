#
# Top level Makefile for openperf
#

all: openperf libopenperf-shim

.PHONY: deps
deps:
	@git submodule init && git submodule update

.PHONY: openperf
openperf: deps
	@cd targets/openperf && $(MAKE)

.PHONY: libopenperf-shim
libopenperf-shim: deps
	@cd targets/libopenperf-shim && $(MAKE)

.PHONY: test
test: test_unit test_aat

.PHONY: test_aat
test_aat: openperf libopenperf-shim
	@cd tests/aat && $(MAKE)

.PHONY: test_unit
test_unit:
	@cd tests/unit && $(MAKE)

.PHONY: clean
clean:
	@cd targets/openperf && $(MAKE) clean
	@cd targets/libopenperf-shim && $(MAKE) clean
	@cd tests/aat && $(MAKE) clean
	@cd tests/unit && $(MAKE) clean
