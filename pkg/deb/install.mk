# Install the openperf binaries into the package staging area.

# Include variables required to flavor the build (this file is created by a makefile
# before calling debuild since debuild does not provide a way to pass through the values)
include ../vars.mk

# Pull in common build variables
OP_ROOT := $(shell pwd)/../../../..
OP_TARGET := package
include $(OP_ROOT)/mk/build.mk

# Values used to identify the openperf binaries
BUILD_DIR = $(OP_ROOT)/build
BUILD_BIN_DIR = $(BUILD_DIR)/openperf-linux-$(ARCH)-$(MODE)/bin
BUILD_LIB_DIR = $(BUILD_DIR)/libopenperf-shim-linux-$(ARCH)-$(MODE)/lib
OP_BIN_NAME = openperf
OP_LIB_NAME = libopenperf-shim.so

.PHONY: all install

all:

install:
	# Install the openperf binaries
	@install -p -v -D -t $(DESTDIR)/usr/bin $(BUILD_BIN_DIR)/$(OP_BIN_NAME)
	@install -p -v -D -t $(DESTDIR)/usr/lib $(BUILD_LIB_DIR)/$(OP_LIB_NAME)

	# Install the openperf configuration file
	@mkdir -p $(DESTDIR)/etc/openperf
	@cp -pf ./config.yaml $(DESTDIR)/etc/openperf/.
