#
# Top level Makefile for inception-core
#

all: devstack

.PHONY: deps
deps:
	@git submodule init && git submodule update

.PHONY: devstack
devstack: deps
	@cd targets/devstack && $(MAKE)

.PHONY: test
test: test_aat

.PHONY: test_aat
test_aat:
	@cd tests/aat && $(MAKE)

.PHONY: clean
clean:
	@cd targets/devstack && $(MAKE) clean
