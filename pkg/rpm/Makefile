# Build the RPM package

# Pull in common build varables
PKG_TYPE := rpm
OP_ROOT ?= $(shell pwd)/../..
OP_TARGET := package-$(PKG_TYPE)
include $(OP_ROOT)/mk/bootstrap.mk

# Include the common packaging definitions
include ../common.mk

# Make sure we have version information to use
$(call op_check_var,VERSION)
$(call op_check_var,PKG_VERSION)
$(call op_check_var,PKG_BASE_NAME)

PKG_NAME := $(PKG_BASE_NAME)_$(VERSION)-$(PKG_VERSION)_$(ARCH)
OP_BIN_NAME := openperf
OP_LIB_NAME := libopenperf-shim
STAGE_DIR := $(OP_BUILD_ROOT)/$(PKG_NAME)
SRC_BIN_DIR := $(OP_ROOT)/build/$(OP_BIN_NAME)-linux-$(ARCH)-$(MODE)/bin
SRC_LIB_DIR := $(OP_ROOT)/build/$(OP_LIB_NAME)-linux-$(ARCH)-$(MODE)/lib
RPM_TGT_DIR := $(STAGE_DIR)/BUILDROOT/$(PKG_BASE_NAME)-$(VERSION)-$(PKG_VERSION).$(ARCH)
TGT_BIN_DIR := $(RPM_TGT_DIR)/usr/bin
TGT_LIB_DIR := $(RPM_TGT_DIR)/usr/lib
TGT_ETC_DIR := $(RPM_TGT_DIR)/etc/$(PKG_BASE_NAME)
PKG_FILE_PATH := $(OP_BUILD_ROOT)/$(PKG_NAME).rpm
TGT_SPEC_FILE := $(STAGE_DIR)/SPECS/$(PKG_BASE_NAME)-$(VERSION).spec

.PHONY: all clean rpm_build_dir

all: $(PKG_FILE_PATH)

# Build the RPM package
$(PKG_FILE_PATH): rpm_build_dir
	cd $(STAGE_DIR) && rpmbuild --define "_topdir $(STAGE_DIR)" -bb $(TGT_SPEC_FILE)

# Setup the RPM build directory
rpm_build_dir: \
		$(TGT_SPEC_FILE) \
		$(TGT_BIN_DIR)/$(OP_BIN_NAME) \
		$(TGT_LIB_DIR)/$(OP_LIB_NAME).so \
		$(TGT_ETC_DIR)/config.yaml \
	# Fill in any directories that RPM expects but may have not been created
	$(foreach d,BUILD BUILDROOT RPMS SOURCES SPECS SRPMS, $(shell mkdir -p $(STAGE_DIR)/$(d)))

# Create the package spec file (replace template parameters and name appropriately)
$(TGT_SPEC_FILE): ./package.spec
	$(call copy_template,$<,$(TGT_SPEC_FILE))

# Get the main openperf binary
$(TGT_BIN_DIR)/$(OP_BIN_NAME): $(SRC_BIN_DIR)/$(OP_BIN_NAME)
	mkdir -p $(dir $@)
	cp -pf $< $@

# Get the openperf shim library
$(TGT_LIB_DIR)/$(OP_LIB_NAME).so: $(SRC_LIB_DIR)/$(OP_LIB_NAME).so
	mkdir -p $(dir $@)
	cp -pf $< $@

# Get the starter configuration file
$(TGT_ETC_DIR)/config.yaml: ./config.yaml
	mkdir -p $(dir $@)
	cp -pf $< $@

# Define used to copy a file to its appropriate location in the build directory in order to
# stage the package build. Template parameters are replaced should they exist in the copied file.
# $1: The path to the file to be copied from.
# $2: The to copy the file to. If any parent directories are missing, they will be created. The
# 	file is copied to preserve file permissions and its timestamp. Any supported template parameters
#	found in the file will be replaced.
define copy_template
	mkdir -p $(dir $(2))
	cp -pf $(1) $(2)
	sed -i \
		-e 's/{{BUILD_ROOT}}/'"$(subst /,\/,$(abspath $(STAGE_DIR)))"'/g' \
		-e 's/{{VERSION}}/'"$(VERSION)"'/g' \
		-e 's/{{PKG_VERSION}}/'"$(PKG_VERSION)"'/g' \
		$(2)
endef

clean:
	rm -rf $(OP_BUILD_ROOT)
