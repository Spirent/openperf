# Common packaging definitions/functionality

# This file expects the following values to be defined (Note:
# the error checking op_check_var define is not available yet)
ifeq ($(PKG_TYPE),)
	$(error PKG_TYPE is not defined)
endif
ifeq ($(OP_ROOT),)
	$(error OP_ROOT is not defined)
endif

# Initialize commonly used values
OP_TARGET := package-$(PKG_TYPE)
include $(OP_ROOT)/mk/bootstrap.mk
include $(OP_ROOT)/mk/versions.mk

# The version of the packaging of the software (could be the same
# version of the software, but changes to the package for example)
export PKG_VERSION ?= 1

# The basename for the generated package
export PKG_BASE_NAME ?= openperf

# Get the version number to be used within the package file name
VERSION := $(shell echo "$(GIT_VERSION)" | sed -e 's/^v//;' -e 's/-.*$$//;')
$(call op_check_var,VERSION)
