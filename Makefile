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

.PHONY: clean
clean:
	@cd targets/devstack && $(MAKE) clean
