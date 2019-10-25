#
# Makefile component for DPDK builds
#

DPDK_REQ_VARS := \
	ICP_ROOT \
	ICP_BUILD_ROOT \
	ICP_DPDK_TARGET \
	ARCH
$(call icp_check_vars,$(DPDK_REQ_VARS))

DPDK_SRC_DIR := $(ICP_ROOT)/deps/dpdk
DPDK_BLD_DIR := $(ICP_BUILD_ROOT)/dpdk
DPDK_INC_DIR := $(DPDK_BLD_DIR)/include
DPDK_LIB_DIR := $(DPDK_BLD_DIR)/lib

DPDK_DEFCONFIG := $(ICP_ROOT)/conf/dpdk/defconfig_$(ARCH)-$(ICP_DPDK_TARGET)

# DPDK target depends on the configuration we're building
DPDK_SHARED = $(shell grep "BUILD_SHARED_LIB=y" $(DPDK_DEFCONFIG))
ifneq (,$(DPDK_SHARED))
	DPDK_TARGET := $(DPDK_LIB_DIR)/libdpdk.so
else
	DPDK_TARGET := $(DPDK_LIB_DIR)/libdpdk.a
endif

# Generate appropriate DPDK_LDLIBS based on the dpdk defconfig
include $(ICP_ROOT)/mk/dpdk_ldlibs.mk

# Generate appropriate DPDK_DEFINES based on the current compiler flags
include $(ICP_ROOT)/mk/dpdk_defines.mk

# Update global build variables
ICP_INC_DIRS += $(DPDK_INC_DIR)
ICP_LIB_DIRS += $(DPDK_LIB_DIR)
ICP_LDLIBS += $(DPDK_LDLIBS) -pthread -ldl
ICP_DEFINES += $(DPDK_DEFINES)

###
# DPDK build rules
###

$(DPDK_BLD_DIR)/.config:
	cp $(DPDK_DEFCONFIG) $(DPDK_SRC_DIR)/config
	cd $(DPDK_SRC_DIR) && $(MAKE) config T=$(ARCH)-$(ICP_DPDK_TARGET) O=$(DPDK_BLD_DIR)

$(DPDK_TARGET): $(DPDK_BLD_DIR)/.config
	cd $(DPDK_BLD_DIR) && $(MAKE) EXTRA_CFLAGS="$(strip $(ICP_CSTD) $(ICP_COPTS))"

.PHONY: dpdk
dpdk: $(DPDK_TARGET)

.PHONY: clean_dpdk
clean_dpdk:
	@rm -f $(DPDK_SRC_DIR)/config/$(notdir $(DPDK_DEFCONFIG))
	@rm -rf $(DPDK_BLD_DIR)
clean: clean_dpdk
