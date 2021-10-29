#
# Makefile component for proper DPDK defines
#

DPDK_COMPILER_DEFINES := $(shell $(OP_CC) $(OP_COPTS) $(OP_CROSSFLAGS) -dM -E - < /dev/null)

###
# arm flags
###
ifneq ($(filter $(DPDK_COMPILER_DEFINES),__aarch64__),)
DPDK_CONFIG_FLAGS += FORCE_INTRINSICS
endif

undefine DPDK_COMPILER_DEFINES

DPDK_DEFINES += \
       $(addprefix -DRTE_, $(DPDK_CONFIG_FLAGS))
