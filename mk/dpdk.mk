#
# Makefile component for DPDK builds
#

DPDK_REQ_VARS := \
	OP_ROOT \
	OP_BUILD_ROOT \
	ARCH \
	PLATFORM
$(call op_check_vars,$(DPDK_REQ_VARS))

# Need these to generate a comma separated list
DPDK_COMMA := ,
DPDK_EMPTY :=
DPDK_SPACE := $(PGA_EMPTY) $(PGA_EMPTY)

DPDK_SRC_DIR := $(OP_ROOT)/deps/dpdk
DPDK_BLD_DIR := $(OP_BUILD_ROOT)/obj/dpdk
DPDK_TGT_DIR := $(OP_BUILD_ROOT)/dpdk
DPDK_INC_DIR := $(DPDK_TGT_DIR)/include
DPDK_LIB_DIR := $(DPDK_TGT_DIR)/lib

DPDK_DRIVERS := \
	mempool/bucket \
	mempool/ring \
	mempool/stack \
	net/af_packet \
	net/bonding \
	net/e1000 \
	net/i40e \
	net/ixgbe \
	net/ring \
	net/virtio \
	net/vmxnet3

# Mellanox ConnectX-3 infiniband library
HAS_MLX4 ?= $(call op_check_system_lib, libmlx4)
ifeq ($(HAS_MLX4),1)
	DPDK_DRIVERS += net/mlx4
endif

# Mellanox ConnectX-4/5/6 infiniband library
HAS_MLX5 ?= $(call op_check_system_lib, libmlx5)
ifeq ($(HAS_MLX5),1)
	DPDK_DRIVERS += bus/auxiliary common/mlx5 net/mlx5 crypto/mlx5 compress/mlx5 regex/mlx5 vdpa/mlx5
endif

# The meson build doesn't support LTO with clang :(
DPDK_CC := clang

# Meson build needs a proper install directory so pkg-config can tell us all
# the libraries we need when linking
DPDK_MESON_OPTS := --prefix=$(DPDK_TGT_DIR) \
	-Db_staticpic=true \
	-Dc_std=gnu11 \
	-Denable_driver_sdk=true \
	-Denable_drivers=$(call op_make_comma_delimited,$(DPDK_DRIVERS)) \
	-Dlibdir=lib \
	-Dtests=false

DPDK_C_ARGS :=

ifeq ($(MODE),debug)
  DPDK_MESON_OPTS += -Dbuildtype=debug
else
	DPDK_MESON_OPTS += -Doptimization=s
  ifeq ($(MODE),testing)
		DPDK_C_ARGS += -gline-tables-only
	endif
endif

ifneq ($(ARCH), $(HOST_ARCH))
  # Cross compiling
	ifeq ($(ARCH), aarch64)
		DPDK_MESON_OPTS += \
			--cross-file $(OP_ROOT)/conf/dpdk/arm64_armv8_linux_clang_openperf \
			-Dc_link_args="$(OP_CROSSFLAGS) $(OP_LDOPTS)"
		DPDK_C_ARGS += $(OP_CROSSFLAGS)
  endif
endif

DPDK_MESON_OPTS += -Dc_args="$(DPDK_C_ARGS)"

ifneq (,$(OP_MACHINE_ARCH))
	DPDK_MESON_OPTS += -Dcpu_instruction_set=$(OP_MACHINE_ARCH)
endif

# DPDK requires the use of pkg-config to determine libraries, however
# we can't access that file until *after* we build and install the
# libraries. Hence, defer this variable expansion until needed.
OP_DPDK_LDLIBS = $(shell PKG_CONFIG_PATH=$(DPDK_LIB_DIR)/pkgconfig pkg-config --libs --static libdpdk 2> /dev/null)

# Update global build variables
OP_INC_DIRS += -include rte_config.h $(DPDK_INC_DIR)
OP_LIB_DIRS += $(DPDK_LIB_DIR)

# Generate appropriate DPDK_DEFINES based on the current build
include $(OP_ROOT)/mk/dpdk_defines.mk
OP_DEFINES += $(DPDK_DEFINES)

###
# DPDK build rules
###

$(DPDK_BLD_DIR)/build.ninja: $(DPDK_DEFCONFIG)
	cd $(DPDK_SRC_DIR) && CC="$(DPDK_CC)" meson $(DPDK_MESON_OPTS) $(DPDK_BLD_DIR)

# Sigh. Ninja doesn't seem to play well from make, so we don't
# have any good way to limit the number of parallel builds here...
$(DPDK_LIB_DIR): $(DPDK_BLD_DIR)/build.ninja
	cd $(DPDK_BLD_DIR) && ninja install

.PHONY: dpdk
dpdk: $(DPDK_LIB_DIR)

.PHONY: clean_dpdk
clean_dpdk:
	@rm -rf $(DPDK_BLD_DIR) $(DPDK_TGT_DIR)
clean: clean_dpdk
