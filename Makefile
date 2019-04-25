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
test: test_aat

.PHONY: test_aat
test_aat:
	@cd tests/aat && $(MAKE)

.PHONY: test_unit
test_unit:
	@cd tests/unit && $(MAKE)

.PHONY: clean
clean:
	@cd targets/inception && $(MAKE) clean
	@cd tests/unit && $(MAKE) clean
