# Build the debian package

# Pull in common build varables
PKG_TYPE := debian
OP_ROOT := $(shell pwd)/../..
OP_TARGET := package-$(PKG_TYPE)
include $(OP_ROOT)/mk/bootstrap.mk

# Make sure we have version information to use
$(call op_check_var,VERSION)
$(call op_check_var,PKG_VERSION)
$(call op_check_var,PKG_BASE_NAME)

# Adjust the package architecture name (if needed)
PKG_ARCH := $(ARCH)
ifeq ($(ARCH),x86_64)
	PKG_ARCH := amd64
endif

PKG_NAME := $(PKG_BASE_NAME)_$(VERSION)-$(PKG_VERSION)_$(PKG_ARCH)
STAGE_DIR := $(OP_BUILD_ROOT)/$(PKG_NAME)
STAGE_PKG_DIR := $(STAGE_DIR)/$(PKG_BASE_NAME)-$(VERSION)
TEMPLATES_DIR := ./templates
TEMPLATES_SRCS := $(shell find $(TEMPLATES_DIR) -type f)
TEMPLATES_DSTS := $(subst $(TEMPLATES_DIR),$(STAGE_PKG_DIR)/$(PKG_TYPE),$(TEMPLATES_SRCS))
INSTALLER_MK := $(STAGE_PKG_DIR)/Makefile
SRC_TAR := $(STAGE_DIR)/$(PKG_BASE_NAME)-$(VERSION).tar.gz
CONFIG_FILE := $(STAGE_PKG_DIR)/config.yaml
PKG_FILE_PATH := $(OP_BUILD_ROOT)/$(PKG_NAME).deb
VARS_MK := $(STAGE_DIR)/vars.mk

.PHONY: all clean

all: $(PKG_FILE_PATH)

# Use a make file to copy files from the openperf build
$(INSTALLER_MK): install.mk
	@mkdir -p $(dir $@)
	@cp -pf $< $@

# Use a default openperf configuration file
$(CONFIG_FILE): ./config.yaml
	@mkdir -p $(dir $@)
	@cp -pf $< $@

# Pass variables to the makefile called from debuild
$(VARS_MK):
	mkdir -p $(dir $@)
	echo "MODE=$(MODE)" > $@

# Setup an archive the way the Debian packaging tools expect
$(SRC_TAR): $(INSTALLER_MK) $(CONFIG_FILE) $(TEMPLATES_DSTS) $(VARS_MK)
	@mkdir -p $(dir $@)
	@cd $(STAGE_DIR) && tar -czf $@ $(PKG_BASE_NAME)-$(VERSION)
	@ln -sf $@ $(STAGE_DIR)/$(PKG_BASE_NAME)_$(VERSION).orig.tar.gz

# Build the Debian package
$(PKG_FILE_PATH): $(SRC_TAR)
	@cd $(STAGE_PKG_DIR) && debuild
	@cp -pf $(STAGE_DIR)/$(notdir $@) $@

# Define used to copy a file to its appropriate location in the build directory in order to
# stage the package build. Template parameters are replaced should they exist in the copied file.
# $1: The path to the file to be copied from.
# $2: The to copy the file to. If any parent directories are missing, they will be created. The
# 	file is copied to preserve file permissions and its timestamp. Any supported template parameters
#	found in the file will be replaced.
define copy_template
$(2) : $(1)
	@mkdir -p $(dir $(2))
	@cp -pf $(1) $(2)
	@sed -i \
		-e 's/{{VERSION}}/'"$(VERSION)"'/g' \
		-e 's/{{PKG_VERSION}}/'"$(PKG_VERSION)"'/g' \
		-e 's/{{ARCHITECTURE}}/'"$(PKG_ARCH)"'/g' \
		-e 's/{{COPYRIGHT_YEAR}}/'"$(shell date +%Y)"'/g' \
		-e 's/{{TIMESTAMP}}/'"$(shell date +"%a, %d %b %Y %X %z")"'/g' \
    	$(2)
endef

# Create all the rules needed to copy the package template files into the build directory
$(foreach _f,$(TEMPLATES_SRCS),$(eval $(call copy_template,$(_f),\
	$(subst $(TEMPLATES_DIR),$(STAGE_PKG_DIR)/$(PKG_TYPE),$(_f)))))

clean:
	@rm -rf $(OP_BUILD_ROOT)
