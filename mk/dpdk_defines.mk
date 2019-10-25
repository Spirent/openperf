#
# Makefile component for proper DPDK 'machine' defines
#
# Note: this only generates a subset of DPDK's cpu related defines as we only
# need the ones that influence the behavior of rte_memcpy.
#

DPDK_COMPILER_DEFINES := $(shell $(ICP_CC) $(ICP_COPTS) -dM -E - < /dev/null)

###
# x86 flags
###
DPDK_MACHINE_FLAGS :=

ifneq ($(filter $(DPDK_COMPILER_DEFINES),__AVX__),)
DPDK_MACHINE_FLAGS += AVX
endif

ifneq ($(filter $(DPDK_COMPILER_DEFINES),__AVX2__),)
DPDK_MACHINE_FLAGS += AVX2
endif

ifneq ($(filter $(DPDK_COMPILER_DEFINES),__AVX512F__),)
DPDK_MACHINE_FLAGS += AVX512F
endif

undefine DPDK_COMPILER_DEFINES

DPDK_DEFINES := $(addprefix -DRTE_MACHINE_CPUFLAG_,$(DPDK_MACHINE_FLAGS))
