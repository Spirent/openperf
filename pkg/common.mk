# Common packaging definitions

# This file expects the following values to be defined
# (Note: the handle op_check_var define is not available yet)
ifeq ($(PKG_TYPE),)
	$(error PKG_TYPE is not defined)
endif
ifeq ($(OP_ROOT),)
	$(error OP_ROOT is not defined)
endif

# Initialize commonly used values
OP_TARGET := package-$(PKG_TYPE)
include $(OP_ROOT)/mk/bootstrap.mk

# Get the version to use for the packaging from the output of the openperf command (Note: If
# the command does not exist, we still set the version so goals such as clean will work. The
# version is set to NO_VERSION in this case so it can be detected when no version is available)
export NO_VERSION := NO_VERSION
VERSION_SRC	:= $(abspath $(OP_ROOT)/build/openperf-$(PLATFORM)-$(ARCH)-$(MODE)/bin/openperf)
ifeq ($(wildcard $(VERSION_SRC)),$(VERSION_SRC))
	VERSION := $(shell $(VERSION_SRC) --version 2>&1 | head -n 1 \
		| sed -e 's/^.*version v//g;' -e 's/-.*$$//;')
else
	VERSION := $(NO_VERSION)
endif

# Create a role that can be used to verify that a package version has been obtained
.PHONY: version_check
version_check:
	@if [[ -z "$(VERSION)" || "$(VERSION)" == "$(NO_VERSION)" ]]; then \
		echo "ERROR: Unable to obtain the packaging version number"; \
		exit 1; \
	fi
