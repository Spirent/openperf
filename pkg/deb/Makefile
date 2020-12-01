# Build the debian package

# Pull in common build variables
PKG_TYPE := deb
OP_ROOT := $(abspath $(shell pwd)/../..)
include $(OP_ROOT)/pkg/common.mk

# Make sure we have version information to use
$(call op_check_vars,VERSION PKG_VERSION PKG_BASE_NAME)

# Adjust the package architecture name (if needed)
PKG_ARCH := $(ARCH)
ifeq ($(ARCH),x86_64)
	PKG_ARCH := amd64
else
	$(error "Unsupported architecture: $(ARCH))
endif

PKG_NAME := $(PKG_BASE_NAME)_$(VERSION)-$(PKG_VERSION)_$(PKG_ARCH)
STAGE_DIR := $(OP_BUILD_ROOT)/$(PKG_NAME)
STAGE_PKG_DIR := $(STAGE_DIR)/$(PKG_BASE_NAME)-$(VERSION)
TEMPLATES_DIR := ./templates
TEMPLATES_SRCS := $(shell find $(TEMPLATES_DIR) -type f)
TEMPLATES_DSTS := $(subst $(TEMPLATES_DIR),$(STAGE_PKG_DIR)/debian,$(TEMPLATES_SRCS))
INSTALLER_MK := $(STAGE_PKG_DIR)/Makefile
SRC_TAR := $(STAGE_DIR)/$(PKG_BASE_NAME)-$(VERSION).tar.gz
CONFIG_FILE := $(STAGE_PKG_DIR)/config.yaml
CHANGELOG_FILE := $(STAGE_PKG_DIR)/debian/changelog
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

# Generate the changelog from the git log content (the generated file
# will be line wrapped at 80 characters to avoid lintian warnings)
$(CHANGELOG_FILE):
	@mkdir -p $(dir $@)
	@./gen-changelog.sh "${PKG_VERSION}" | fmt -w80 -s > $@

# Pass variables to the makefile called from debuild
$(VARS_MK):
	mkdir -p $(dir $@)
	echo "MODE=$(MODE)" > $@

# Build the Debian package
$(PKG_FILE_PATH): $(INSTALLER_MK) $(CONFIG_FILE) $(CHANGELOG_FILE) $(TEMPLATES_DSTS) $(VARS_MK)
	@cd $(STAGE_PKG_DIR) && debuild --no-tgz-check -b -uc -us
	@cp -pf $(STAGE_DIR)/$(notdir $@) $@

# Define used to copy a file to its appropriate location in the build directory in order to
# stage the package build. Template parameters are replaced should they exist in the copied file.
# $1: The path to the file to be copied from.
# $2: The to copy the file to. If any parent directories are missing, they will be created. The
#    file is copied to preserve file permissions and its timestamp. Any supported template parameters
#    found in the file will be replaced.
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
	$(subst $(TEMPLATES_DIR),$(STAGE_PKG_DIR)/debian,$(_f)))))

clean:
	@rm -rf $(OP_BUILD_ROOT)
