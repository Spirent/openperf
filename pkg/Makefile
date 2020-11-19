
# SM TODO: Not sure where this is to come from
VERSION ?= 0.1.0
PKG_VERSION ?= 1

# Pull in common build varables (requires OP_ROOT to be defined)
OP_TARGET := package
OP_ROOT := $(shell pwd)/..
include $(OP_ROOT)/mk/bootstrap.mk

# Variables to pass to the submake calls
SUBMK_VARS = \
	MODE=$(MODE) \
	VERSION=$(VERSION) \
	PKG_VERSION=$(PKG_VERSION) \
	PKG_BASE_NAME=openperf

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
	cd debian && $(MAKE) all $(SUBMK_VARS)

package-rpm:
	cd rpm && $(MAKE) all $(SUBMK_VARS)

clean-debian:
	cd debian && $(MAKE) clean $(SUBMK_VARS)

clean-rpm:
	cd rpm && $(MAKE) clean $(SUBMK_VARS)

clean: clean-debian clean-rpm
