# Install the openperf binaries into the package staging area.

# Include variables required to flavor the build (this file is created by a makefile
# before calling debuild since debuild does not provide a way to pass through the values)
include ../vars.mk

# Pull in common build variables
OP_ROOT := $(shell pwd)/../../../..
OP_TARGET := package
include $(OP_ROOT)/mk/build.mk

# Values used to identify the openperf binaries
BUILD_DIR := $(abspath $(OP_ROOT)/build)
BUILD_BIN_DIR := $(abspath $(BUILD_DIR)/openperf-linux-$(ARCH)-$(MODE)/bin)
BUILD_LIB_DIR := $(abspath $(BUILD_DIR)/libopenperf-shim-linux-$(ARCH)-$(MODE)/lib)
OP_BIN_NAME := openperf
OP_LIB_NAME := libopenperf-shim.so
BUILD_PLUGINS_DIR := $(abspath $(dir $(BUILD_BIN_DIR))/plugins)
PLUGINS_SRC_FILES := $(shell find $(BUILD_PLUGINS_DIR) -type f -name "*.so")
PLUGINS_TGT_DIR := $(abspath $(DESTDIR)/usr/lib/openperf/plugins)
PLUGINS_TGT_FILES := $(subst $(BUILD_PLUGINS_DIR)/,$(PLUGINS_TGT_DIR)/,$(PLUGINS_SRC_FILES))

.PHONY: all install

all:

install: $(PLUGINS_TGT_FILES)
	# Install the openperf binaries
	@install -p -v -D -t $(DESTDIR)/usr/bin $(BUILD_BIN_DIR)/$(OP_BIN_NAME)
	@install -p -v -D -t $(DESTDIR)/usr/lib $(BUILD_LIB_DIR)/$(OP_LIB_NAME)

	# Install the openperf configuration file
	@mkdir -p $(DESTDIR)/etc/openperf
	@cp -pf ./config.yaml $(DESTDIR)/etc/openperf/.

# Define rules to install to a target location.
# $1: The path to the file to be copied from.
# $2: The target directory to which the file will be copied.
define install_file_rule
$(2)/$(notdir $(1)): $(1)
	@install -p -v -D -t $(2) $(1)
endef

# Create rules to copy the plugins to their destination in the package staging directory
$(foreach _f,$(PLUGINS_SRC_FILES),$(eval $(call install_file_rule,$(_f),$(PLUGINS_TGT_DIR))))
